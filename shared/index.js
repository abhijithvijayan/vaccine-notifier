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

function getFirstAvailableSession(groups, sessions) {
  return groups
    .map((group) => {
      return sessions.find(({available_capacity_dose1, min_age_limit}) => {
        return available_capacity_dose1 > 0 && group === min_age_limit;
      });
    })
    .filter(Boolean);
}

function getCenters(centers) {
  const matches = [...centers.matchAll(CENTERS_REGEX)];

  return matches.reduce((acc, {1: centerId, 2: centerName}) => {
    acc[centerId] = centerName.trim();

    return acc;
  }, {});
}

function extractAgeGroups(groups) {
  return groups.split(',').filter(Boolean).map(Number);
}

module.exports = {
  ageGroups,
  adjustForTimezone,
  timezoneOffset,
  CENTERS_REGEX,
  getCurrentDate,
  getFirstAvailableSession,
  getCenters,
  extractAgeGroups
}