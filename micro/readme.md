# vaccine-notifier

## Docs

### Set up deta.sh

> <https://docs.deta.sh/docs/cli/install>

### Deploy to Deta

```shell
deta new --node cowin
yarn install
yarn deploy
yarn set:cron # sets for 2 minutes
```

#### Note: For subsequent deployment run only

```shell
yarn deploy
```

### Create Slack Bot For Workspace

> <https://slack.com/intl/en-in/help/articles/115005265703-Create-a-bot-for-your-workspace>

### Creating Telegram Bot

> <https://core.telegram.org/bots#6-botfather>

### Getting Telegram User Id for account

> Search for `@getidsbot` in telegram
> Click Start
> Use the `id` field returned
