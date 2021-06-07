const {get} = require('@abhijithvijayan/ts-utils');
const fetch = require('node-fetch');

const timezoneOffset = 5.5; // GMT+5:30

const {
  TELEGRAM_CHAT_ID = '',
  TELEGRAM_BOT_TOKEN = '',
  SLACK_WEBHOOK_URL = '',
} = process.env;

function adjustForTimezone(date, offset = 0) {
  const timeOffsetInMS = offset * 60 * 60 * 1000;
  date.setTime(date.getTime() + timeOffsetInMS);

  return date;
}

function currentDate() {
  const today = adjustForTimezone(new Date(), timezoneOffset);
  const dd = today.getDate();
  const mm = today.getMonth() + 1;
  const yyyy = today.getFullYear();

  return `${dd}-${mm}-${yyyy}`;
}

module.exports.getCurrentDate = async () => {
  return {
    statusCode: 200,
    body: currentDate(),
  };
};

module.exports.notifyIfAvailable = async () => {
  const slots = [];

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

  return {
    statusCode: 200,
  };
};
