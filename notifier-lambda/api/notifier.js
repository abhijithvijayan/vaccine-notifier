function adjustForTimezone(date, offset = 0) {
  const timeOffsetInMS = offset * 60 * 60 * 1000;
  date.setTime(date.getTime() + timeOffsetInMS);

  return date;
}

function currentDate() {
  const today = adjustForTimezone(new Date(), 5.5);
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
