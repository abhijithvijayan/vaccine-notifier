const timezoneOffset = 5.5; // GMT+5:30

const CENTERS_REGEX = /<([0-9]+),([a-z0-9A-Z\s,._-]+)>/g;

const ageGroups = {
  BELOW_45: 18,
  BETWEEN_40_AND_45: 40,
  AND_ABOVE_45: 45,
};

function adjustForTimezone(date, offset = 0) {
  const timeOffsetInMS = offset * 60 * 60 * 1000;
  date.setTime(date.getTime() + timeOffsetInMS);

  return date;
}

function getCurrentDate() {
  const today = adjustForTimezone(new Date(), timezoneOffset);
  const dd = today.getDate();
  const mm = today.getMonth() + 1;
  const yyyy = today.getFullYear();

  return `${dd}-${mm}-${yyyy}`;
}

module.exports = {
  ageGroups,
  adjustForTimezone,
  timezoneOffset,
  CENTERS_REGEX,
  getCurrentDate
}