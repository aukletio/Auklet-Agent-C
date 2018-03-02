#!/bin/bash
set -e
THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Get the version and drop all build metadata from it.
VERSION=$(cat ~/.version | xargs | cut -f1 -d"+")
# Revert to a pristine checkout.
echo 'Reverting to pristine checkout...'
git reset --hard HEAD
git clean -qfdx || true
# Create release in JIRA.
JIRA_PROJECT=$(cat .esg | jq -r .jiraProject)
echo "Creating release in JIRA project '$JIRA_PROJECT'..."
JIRA_DATE=$(date +'%d/%b/%Y')
PAYLOAD="{\"name\": \"$CIRCLE_PROJECT_REPONAME $VERSION\", \"released\": true, \"userReleaseDate\": \"$JIRA_DATE\", \"project\": \"$JIRA_PROJECT\"}"
HEADERS_FILE="/tmp/jira-$RANDOM"
BODY_FILE="/tmp/jira-$RANDOM"
set +e
curl -D $HEADERS_FILE -sS -u "$JIRA_USERNAME:$JIRA_PASSWORD" -H "Content-Type: application/json" -X POST -d "$PAYLOAD" "$JIRA_URL/rest/api/2/version" > $BODY_FILE
RESULT=$?
set -e
cat -s $HEADERS_FILE
cat $BODY_FILE | jq .
if [[ "$RESULT" != "0" ]]; then
  exit $RESULT
fi
# Make and push the tag.
echo
echo 'Tagging release...'
git checkout $CIRCLE_BRANCH
git tag -a $VERSION -m "$VERSION"
git push origin $VERSION
TAG_SHA=$(git show-ref -s $VERSION | xargs) # This is NOT the same as the SHA-1 of the commit to which the tag points. Remember that the tag created above is annotated, not lightweight.
# Wait until the GitHub API recognizes the new tag.
# This ensures that the changelog generator will see it.
echo
echo 'Waiting for new tag to appear in GitHub API...'
TAG_RESULT=''
while [ ! "$TAG_RESULT" == "$VERSION" ]; do
  sleep 5
  TAG_RESULT=$(curl -s -H "Authorization: Token $CHANGELOG_GITHUB_TOKEN" https://api.github.com/repos/$CIRCLE_PROJECT_USERNAME/$CIRCLE_PROJECT_REPONAME/git/tags/$TAG_SHA | jq -r .tag)
done
# Switch to the changelog branch.
echo 'Generating changelog updates...'
git checkout changelog
git branch -u origin/changelog changelog
git pull
# Generate the changelogs.
CURRENT_DIR="$(pwd)"
cd ~ # Prevents codebase contamination.
npm install --no-spin bluebird any-promise request-promise-any request semver semver-extra semver-sort parse-link-header > /dev/null 2>&1
node $THIS_DIR/calculateChangelogs.js "$CURRENT_DIR"
eval cd $CURRENT_DIR
# Push the changelog to GitHub.
echo
echo 'Updating changelog and pushing to GitHub...'
git add README.md README-WITH-RC.md
git commit -m "$VERSION [skip ci]"
git push
