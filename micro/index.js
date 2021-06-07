require('dotenv').config({path: `${__dirname}/.env`});
const {get, isNullOrUndefined, isEmpty} = require('@abhijithvijayan/ts-utils');
const fetch = require('node-fetch');
const {app} = require('deta');

const {
  CENTERS_REGEX,
  ageGroups,
  getCurrentDate,
  adjustForTimezone,
  timezoneOffset,
} = require('../shared');

// Note: Rate Limit is 100 Calls Per 5 Minute Per IP

const timeout = 10000; // 10 seconds
const {
  TELEGRAM_CHAT_ID = '',
  VACCINATION_CENTERS = '',
  TELEGRAM_BOT_TOKEN = '',
  SLACK_WEBHOOK_URL = '',
  AGE_CATEGORY = `${ageGroups.BELOW_45},${ageGroups.BETWEEN_40_AND_45},${ageGroups.AND_ABOVE_45}`,
  DISTRICT_CODE = '',
} = process.env;

function getAgeCategoryFromEnv() {
  return AGE_CATEGORY.split(',').filter(Boolean).map(Number);
}

function getCentersFromEnv() {
  const matches = [...VACCINATION_CENTERS.matchAll(CENTERS_REGEX)];

  return matches.reduce((acc, {1: centerId, 2: centerName}) => {
    acc[centerId] = centerName.trim();

    return acc;
  }, {});
}

// These id's will later be converted to numbers
const whiteListedCenters = getCentersFromEnv();

// collect for each group, returns an array
function getFirstAvailableSession(sessions) {
  const ageCategory = getAgeCategoryFromEnv();

  return ageCategory
    .map((group) => {
      return sessions.find(({available_capacity_dose1, min_age_limit}) => {
        return available_capacity_dose1 > 0 && group === min_age_limit;
      });
    })
    .filter(Boolean);
}

function getCenterName(cId) {
  const centerId = `${cId}`;
  const centerName = whiteListedCenters[centerId];
  if (isNullOrUndefined(centerName)) {
    return centerId;
  }

  return centerName;
}

function fetchFromCenter(centerId) {
  const date = getCurrentDate();

  return fetch(
    `https://cdn-api.co-vin.in/api/v2/appointment/sessions/public/calendarByCenter?center_id=${centerId}&date=${date}`,
    {
      method: 'GET',
      timeout,
    }
  ).then((data) => data.json());
}

function fetchFromDistrict(code) {
  const date = getCurrentDate();

  return fetch(
    `https://cdn-api.co-vin.in/api/v2/appointment/sessions/public/calendarByDistrict?district_id=${code}&date=${date}`,
    {
      method: 'GET',
      timeout,
    }
  ).then((data) => data.json());
}

async function fetchSlots() {
  const failedCenters = [];
  const slots = [];

  try {
    const whiteListedCentersIds = Object.keys(whiteListedCenters).map(Number); // conversion to number
    const response = await Promise.allSettled(
      whiteListedCentersIds.map((centerId) => {
        return fetchFromCenter(centerId);
      })
    );
    response.forEach((r, requestIndex) => {
      if (r.status === 'fulfilled') {
        const errorMessage = get(r, 'value.message');
        if (errorMessage) {
          failedCenters.push(whiteListedCentersIds[requestIndex]);
          console.log('[ERROR]: Something went wrong with Center API', {
            err: errorMessage,
          });
        } else {
          const centerData = get(r, 'value.centers', {});
          if (isEmpty(centerData)) {
            failedCenters.push(whiteListedCentersIds[requestIndex]);

            return;
          }

          console.log('[INFO]: Fetched Center Data');
          const centerSessions = get(centerData, 'sessions', []);
          const availableSlots = getFirstAvailableSession(centerSessions);
          if (!isEmpty(availableSlots)) {
            const centerName = getCenterName(centerData.center_id);

            slots.push({
              [centerName.toUpperCase()]: availableSlots,
            });
          }

          return;
        }
      }

      if (r.status === 'rejected') {
        failedCenters.push(whiteListedCentersIds[requestIndex]);
      }
    });

    if (!isEmpty(failedCenters)) {
      // fire district api call and filter from centers
      const [districtResponse] = await Promise.allSettled([
        fetchFromDistrict(DISTRICT_CODE),
      ]);
      if (districtResponse.status === 'fulfilled') {
        const errorMessage = get(districtResponse, 'value.message');
        if (errorMessage) {
          console.log('[ERROR]: Something went wrong with District API', {
            err: errorMessage,
          });
        } else {
          const allCenters = get(districtResponse, 'value.centers', []);
          if (!isEmpty(allCenters)) {
            console.log(`[INFO]: Fetched ${allCenters.length} centers`);

            failedCenters.forEach((failedCenterId) => {
              const foundCentersIndex = allCenters.findIndex(
                (c) => c.center_id === failedCenterId
              );
              // check if center exist in district list of centers
              if (foundCentersIndex !== -1) {
                const centerData = allCenters[foundCentersIndex];
                const centerSessions = get(centerData, 'sessions', []);
                const availableSlots = getFirstAvailableSession(centerSessions);
                if (!isEmpty(availableSlots)) {
                  const centerName = getCenterName(centerData.center_id);

                  slots.push({
                    [centerName.toUpperCase()]: availableSlots,
                  });
                }
              }
            });
          } else {
            console.log('[INFO]: No centers for district found!');
          }
        }
      } else if (districtResponse.status === 'rejected') {
        console.log('[ERROR]: Something went wrong', districtResponse.reason);
      }
    }

    if (!isEmpty(slots)) {
      console.log(`[INFO]: ${slots.length} slots available. 💉`);

      const telegramURL = `https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendMessage`;
      const body = ({format = false} = {}) => {
        return `
          Timestamp: ${new Intl.DateTimeFormat('en-GB', {
            dateStyle: 'full',
            timeStyle: 'long',
          }).format(
            adjustForTimezone(new Date(), timezoneOffset)
          )},\n\nAvailable Centers:  ${JSON.stringify(
          slots,
          null,
          format ? 2 : 0
        )}`;
      };

      const [tgResponse, slackResponse] = await Promise.allSettled([
        fetch(telegramURL, {
          method: 'POST',
          body: JSON.stringify({
            chat_id: TELEGRAM_CHAT_ID,
            text: body({format: true}),
          }),
          headers: {'Content-Type': 'application/json'},
        }),
        fetch(SLACK_WEBHOOK_URL, {
          method: 'post',
          body: JSON.stringify({
            text: body({format: false}),
          }),
          headers: {'Content-Type': 'application/json'},
        }),
      ]);

      if (
        tgResponse.status === 'fulfilled' ||
        slackResponse.status === 'fulfilled'
      ) {
        if (
          get(tgResponse, 'value.ok', false) ||
          get(slackResponse, 'value.ok', false)
        ) {
          console.log('[SUCCESS]: Notification sent.');
        }
      } else {
        console.log('[ERROR]: Failed to send notification');
      }

      return;
    }

    console.log('[INFO]: No slots available. 💉');
  } catch (err) {
    console.log('[ERROR]: Something went wrong', err);
  }
}

function start(_event) {
  return fetchSlots();
}

// https://docs.deta.sh/docs/micros/run#run-and-cron
app.lib.run(start);
app.lib.cron(start);

module.exports = app;
