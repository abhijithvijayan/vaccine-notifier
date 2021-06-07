const {isNullOrUndefined, get, isEmpty} = require('@abhijithvijayan/ts-utils');
const fetch = require('node-fetch');

const {
  getFirstAvailableSession,
  adjustForTimezone,
  extractAgeGroups,
  getCurrentDate,
  ageGroups,
  getCenters,
} = require('../../shared');

const timezoneOffset = 5.5; // GMT+5:30

const {
  TELEGRAM_CHAT_ID = '',
  TELEGRAM_BOT_TOKEN = '',
  SLACK_WEBHOOK_URL = '',
  VACCINATION_CENTERS = '',
  AGE_CATEGORY = `${ageGroups.BELOW_45},${ageGroups.BETWEEN_40_AND_45},${ageGroups.AND_ABOVE_45}`,
} = process.env;

const whiteListedCenters = getCenters(VACCINATION_CENTERS);

module.exports.getCurrentDate = async () => {
  return {
    statusCode: 200,
    body: getCurrentDate(),
  };
};

function getCenterName(cId) {
  const centerId = `${cId}`;
  const centerName = whiteListedCenters[centerId];
  if (isNullOrUndefined(centerName)) {
    return centerId;
  }

  return centerName;
}

module.exports.notifyIfAvailable = async (event, context, callback) => {
  const telegramURL = `https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendMessage`;
  const r = JSON.parse(event.body || {});
  const slots = [];

  const errorMessage = get(r, 'value.message');
  if (errorMessage) {
    callback(
      new Error('[ERROR]: Something went wrong with Center API', {
        err: errorMessage,
      })
    );

    return;
  }

  const centerData = get(r, 'value.centers', {});
  if (isEmpty(centerData)) {
    callback(
      new Error(
        '[ERROR]: Something went wrong with Center API. No data received.',
        {
          err: errorMessage,
        }
      )
    );

    return;
  }

  const centerSessions = get(centerData, 'sessions', []);
  const watchingAgeGroups = extractAgeGroups(AGE_CATEGORY);
  const availableSlots = getFirstAvailableSession(
    watchingAgeGroups,
    centerSessions
  );
  if (!isEmpty(availableSlots)) {
    const centerName = getCenterName(centerData.center_id);

    slots.push({
      [centerName.toUpperCase()]: availableSlots,
    });
  }

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

  const response = {
    statusCode: 200,
  };

  callback(null, response);
};
