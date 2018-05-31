// Setup the GitHub API connection.
require('any-promise/register/bluebird');
const rp = require('request-promise-any');
const plh = require('parse-link-header');
const apiPrefix = 'https://api.github.com/';
const github = rp.defaults({
  baseUrl: apiPrefix,
  headers: {
    'User-Agent': 'esg-usa-bot',
    'Authorization': `Token ${process.env.CHANGELOG_GITHUB_TOKEN}`
  },
  json: true,
  resolveWithFullResponse: true
});

// Include other dependent libraries.
const fs = require('fs');
const Promise = require('any-promise');
const semver = require('semver-extra');
const semverSort = require('semver-sort');

// Grab inputs.
const orgName = process.env.CIRCLE_PROJECT_USERNAME;
const repoName = process.env.CIRCLE_PROJECT_REPONAME;

// Setup global vars.
var eligiblePrs = [];
var rcTags = [];
var prodTags = [];
var unreleasedRcPrs = {
  "breaking": [],
  "enhancement": [],
  "bug": [],
  "devops": []
};
var unreleasedProdPrs = {
  "breaking": [],
  "enhancement": [],
  "bug": [],
  "devops": []
};

// Begin.
console.log('Getting all closed PRs in this repo...');
getPaginated({
  uri: `/repos/${orgName}/${repoName}/pulls?per_page=100&state=closed&sort=updated`
}).then(function(closedPrs) {
  // Not all closed PRs are merged. We only want merged PRs.
  // Also, skip all release PRs.
  console.log('Selecting all merged non-release PRs...');
  eligiblePrs = closedPrs.filter(function(pr) {
    var labels = pr.labels.map(function(label) { return label.name; });
    return pr.merged_at !== null && !labels.includes('release');
  });
  // Sort the list of PRs by number.
  eligiblePrs.sort(function(a, b) { return a.number - b.number; });
  eligiblePrs.reverse();
  // Continue.
  getAllTags();
}).catch(catchPromiseError);

function getAllTags() {
  console.log('Getting all tags in this repo...');
  getPaginated({
    uri: `/repos/${orgName}/${repoName}/tags?per_page=100`
  }).then(function(tags) {
    // Extract the tag names (versions) and sort oldest to newest.
    var tagNames = semverSort.asc(tags.map(function(tag) { return tag.name; }));
    console.log(`Tags found (${tags.length}):\n${JSON.stringify(tagNames, null, 2)}`);
    // Assemble a new list of tags that we know is propertly sorted.
    tagNames.forEach(function(tagName) {
      rcTags.push(tags.find(function(tag) { return tag.name === tagName; }));
    });
    rcTags = rcTags.map(function(tag) { return {
      "name": tag.name,
      "md": `[${tag.name}](https://github.com/${orgName}/${repoName}/tree/${tag.name})`,
      "sha": tag.commit.sha,
      "commits": [],
      "pullRequests": {
        "breaking": [],
        "enhancement": [],
        "bug": [],
        "devops": []
      }
    }; });
    // For each of these tags, get all their commits.
    console.log('Getting commit history of these tags...');
    var promises = [];
    rcTags.forEach(function(tag) {
      promises.push(
        getPaginated({
          uri: `repos/${orgName}/${repoName}/commits?per_page=100&sha=${tag.sha}`
        }).then(function(tagCommits) {
          tag.commits = tagCommits.map(function(commit) { return commit.sha; });
        }).catch(catchPromiseError)
      );
    });
    Promise.each(promises, (i, x, l) => i).then(function() {
      // Get a deep clone of the RC tags array and drop all non-RC tags from it.
      prodTags = JSON.parse(JSON.stringify(rcTags));
      prodTags = prodTags.filter(function(tag) { return semver.isStable(tag.name); });
      // Continue.
      matchPrsWithTags();
    });
  }).catch(catchPromiseError);
}

function matchPrsWithTags() {
  // Associate each PR with a prod tag and an RC tag.
  // PRs that are not included in any tag go into a separate bucket.
  console.log('Matching PRs with tags...');
  eligiblePrs.forEach(function(pr) {
    // Determine this PR's type.
    var labels = pr.labels.map(function(label) { return label.name; });
    var type = '';
    if (labels.includes('breaking')) type = 'breaking';
    else if (labels.includes('enhancement')) type = 'enhancement';
    else if (labels.includes('devops')) type = 'devops';
    else type = 'bug';
    // Render the Markdown for this PR.
    var md = `${pr.title} [\#${pr.number}](${pr.html_url}) ([${pr.user.login}](${pr.user.html_url}))`;
    // Sort this PR into the correct tag, in both lists.
    // We are iterating the tags from oldest to newest, so we always associate PRs with the newest tags to which they belong.
    // For the RC tag list, this important because it ensures that PRs are associated with the RC tag and not the corresponding prod tag.
    var rcTag = false;
    rcTags.forEach(function(tag) {
      if (!rcTag && tag.commits.includes(pr.merge_commit_sha)) {
        rcTag = true;
        tag.pullRequests[type].push(md);
      }
    });
    if (!rcTag) unreleasedRcPrs[type].push(md);
    prodTag = false;
    prodTags.forEach(function(tag) {
      if (!prodTag && tag.commits.includes(pr.merge_commit_sha)) {
        prodTag = true;
        tag.pullRequests[type].push(md);
      }
    });
    if (!prodTag) unreleasedProdPrs[type].push(md);
  });
  // Drop the list of tag commits.
  for (let tag of rcTags) {
    delete tag.sha;
    delete tag.commits;
  }
  for (let tag of prodTags) {
    delete tag.sha;
    delete tag.commits;
  }
  // Reverse the tag lists so we render Markdown from newest to oldest.
  rcTags.reverse();
  prodTags.reverse();
  // Render the changelogs and write them to disk.
  console.log('Assembling changelog Markdown...');
  var rcMd = renderMarkdown(unreleasedRcPrs, rcTags);
  var prodMd = renderMarkdown(unreleasedProdPrs, prodTags);
  fs.writeFileSync('README-WITH-RC.md', rcMd);
  fs.writeFileSync('README.md', prodMd);
  // Done.
}

// Utility functions

// Function that does paginated GET requests on the GitHub API
// and returns all relevant results in an array.
function getPaginated(options, resultList = []) {
  return new Promise(function(resolve, reject) {
    github.get(options).then(function(response) {
      if (response.statusCode === 200) {
        // Add these results to our list.
        resultList = resultList.concat(response.body);
        // Follow pagination if necessary.
        var next = '';
        if (response.headers.hasOwnProperty('link')) {
          next = plh(response.headers.link).next;
          if (next) next = next.url;
          if (next) next = next.replace(apiPrefix, '');
        }
        if (next) {
          var newOptions = Object.assign({}, options);
          newOptions.uri = next;
          // Wait for 1 second before getting the next page, to avoid abuse rate limits.
          setTimeout(function() {
            getPaginated(newOptions, resultList).then(function(newResultList) {
              resolve(newResultList);
            });
          }, 1000);
        } else {
          resolve(resultList);
        }
      } else {
        reject(getHttpError(response));
      }
    }).catch(function(e) { reject(e); });
  });
}
// Markdown rendering functions.
function renderMarkdown(unreleasedPrs, tags) {
  var md = '# Changelog\n\n';
  // List the unreleased (untagged) PRs first.
  // Skip this header if there are no such PRs to list.
  var unreleasedHeader = false;
  for (let type of ["breaking", "enhancement", "bug", "devops"]) {
    if (!unreleasedHeader && unreleasedPrs[type].length > 0) {
      unreleasedHeader = true;
      md += '## Merged But Unreleased\n\n';
    }
  }
  for (let type of ["breaking", "enhancement", "bug", "devops"]) {
    md += renderPrMarkdown(unreleasedPrs, type);
  }
  // Now list all tag PRs.
  for (let tag of tags) {
    // Use ### for RC tags and ## for prod tags.
    if (semver.isPrerelease(tag.name)) md += '#';
    md += `## ${tag.md}\n\n`;
    for (let type of ["breaking", "enhancement", "bug", "devops"]) {
      md += renderPrMarkdown(tag.pullRequests, type);
    }
  }
  // Make sure we have exactly one trailing newline.
  md = md.trim() + "\n";
  return md;
}
function renderPrMarkdown(prs, type) {
  var md = '';
  if (prs[type].length > 0) {
    var header = 'Other merged PRs:';
    switch (type) {
      case "breaking":
        header = "Breaking changes:";
        break;
      case "enhancement":
        header = "Implemented enhancements:";
        break;
      case "bug":
        header = "Fixed bugs:";
        break;
      case "devops":
        header = "DevOps changes:";
        break;
    }
    md += `**${header}**\n\n`;
    for (let pr of prs[type]) {
      md += `- ${pr}\n`
    }
    md += '\n'
  }
  return md;
}
// Returns an error object from an HTTP response object.
function getHttpError(response) {
  return new Error(`HTTP request failure: ${response.statusCode} ${response.statusMessage}`)
}
// Logs promise errors and tells NodeJS to fail.
function catchPromiseError(err) {
  console.log('Error occurred during promise');
  console.log(err, err.stack);
  process.exitCode = 1;
}
