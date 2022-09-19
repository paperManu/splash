# RFCs

RFC (Request for Comment) is a way for Splash contributors to propose, design, and discuss new features and changes in project direction in a focused environment.

Once accepted by the maintainer, the related merge request(s) will be associated with the RFC, until completion.

## What does an RFC do?

The RFC process is intended to bring focus and structure to discussions of important changes and new features in the Splash project. It provides a space for sharing and collaborating on ideas and design documents, keeping community discussion organized and focused on the goal of reaching a decision.

Visibility and inclusivity are important values for RFCs. They are advertised to both developer and user communication channels so everyone can participate.

An RFC is only a design document, and not a single line of code needs to be written for the discussion to be approved (although prototyping on a branch is often a good idea). An approved RFC has no timeline for when it needs to be implemented.

## When should I file an RFC?

If you have an idea for something and are unsure if it needs an RFC, the best way to find out is to just ask! An RFC is much more likely to succeed if it begins life as a discussion, and collaborators are encouraged to start off by talking to other project members before submitting an RFC. This can also help gauge early support for the feature and work out any obvious issues or design decisions.

RFCs are used for proposing "substantial" changes. The meaning of "substantial" is subjective, but a good start would be anything that benefits from design discussion. Put a different way, it's recommended to file an RFC if the design or implementation isn't immediately clear, or if there are drawbacks or potential consequences that require discussion first. These might be worth consideration for an RFC:

- New features
- Breaking changes
- Organizational changes, including changes to the RFC process itself

The following do not need to go through an RFC:

- Bug fixes
- Releases of new Splash versions
- Anything unlikely to be controversial or in need of discussion

## Proposing an RFC

### Step 0: Decide if you should file an RFC

As an RFC author, it's best if you only have one RFC of yours open at any moment. This restriction is a guideline, but you are advised to be respectful of other contributors who would also like their RFCs to receive attention, and to be cognizant of the potential cost of spreading the community's attention thinly across a multitude of proposals.

### Step 1: File a RFC issue

When creating the issue, in the "Description" entry, select the "rfc" template.

Please put a lot of thought into your writing. A great RFC explains and justifies its motivations and carefully considers all drawbacks and alternatives. Show the reader what the problems are and how the proposal concretely and practically addresses them.

### Step 2: Announce

After the RFC is ready to go, please announce it to the project communication channels, such as mailing lists and chat

If you don't have an account in some of these places, someone else can do it for you if you make a request in the RFC submission.

Here is a template:

```
Subject: RFC: Deprecate BadFeature

A new RFC (request for comment) has been opened here:

https://gitlab.com/sat-mtl/tools/splash/splash/-/issues/333

Please visit the above link for discussion.

Summary: <insert the summary section from your RFC here>
```

## Discussing an RFC

Discussion about the RFC should happen on the RFC pull request on GitLab. If discussions happen outside the GitLab thread, it's best to link and summarize them in the RFC thread to make the conversations easier to track.

The RFC will likely undergo numerous revisions in the discussion process.

While discussing an RFC, it is everyone's responsibility to work towards **reaching a conclusion**. When you make a post to the RFC thread, focus on the contents and implications of the RFC only.

Here are some guidelines:

- **Think about these important questions:**
  - Do you agree with the proposal in its current form?
  - Are concerns and questions adequately addressed?
  - Do you have suggestions for how the RFC can be revised or expanded, especially in the Unresolved Questions section?
- **Don't be shy:** Even if you aren't an active Splash contributor, or if you don't feel "qualified" to discuss the topic, please feel free to directly state your opinion on an RFC, especially if it affects you in some way.
- **Stay on topic:** Starting or continuing tangential or off-topic discussions is disrespectful to the RFC author and is strongly discouraged. Such discussions often dilute the thread and make it hard to identify what the community's decision should be. The purpose of the RFC thread is to reach a conclusion, and any commentary that does not contribute to that is damaging to the RFC process.
- **Civility and respect:** Like all other discussions that happen in the Splash community, RFC discussions are governed by our [Code of Conduct](./code-of-conduct.md). Creating an RFC is a laborious project. Commenting on an RFC should be done with respect and empathy for the hard work that the author has put into it.
- **Avoid scope creep:** You should make suggestions on how the RFC can be revised or expanded, but be careful not to expand it too much and develop it into something far more ambitious than was originally intended.

## Finalizing an RFC

When the RFC seems ready to conclude, anyone may make a motion for a Final Comment Period. The Final Comment Period lasts 14 days, and has a *disposition* to either **pass** or **reject**.

To make a motion for Final Comment Period, simply leave a comment on the pull request thread. The Final Comment Period should also be announced on the same forums as above. Here is a template:

```
Subject: RFC Final Comment Period: Deprecate BadFeature

An RFC has now entered Final Comment Period. In 14 days, discussion will end and the proposal will either be accepted, rejected or withdrawn:

https://gitlab.com/sat-mtl/tools/splash/splash/-/issues/333

Please visit the above link for discussion.

Summary: <insert the summary section from your RFC here>
```

When should the Final Comment Period start? There doesn't need to be a consensus with everyone involved (which is usually impossible), but the RFC should be in a state where the author feels that the writing effort is complete, the arguments have been clearly articlated, discussions are resolved and drawbacks acknowledged in the RFC's text, and there isn't a strong consensus *against* the proposal.

Sometimes, RFCs can reach points of disagreement where no clear consensus is in sight. Unfortunately, there is no way that this RFC process can solve such issues, and these have to be handled on a case-by-case basis.

If no important developments occur during this time frame, someone with write access merges (for passed RFCs) or closes (for rejected RFCs) the RFC PR.

At any point, the RFC author can also choose to **withdraw** an RFC, with no 14-day grace period necessary.

Implementation of an RFC can start before, during, or after it has been approved. In fact, if you propose an RFC, you don't need to be the person to implement it, but usually the implementer is the same person as the RFC author.

## About the RFC process

This isn't a legal document, and you shouldn't spend a lot of time analyzing its wording. This process is intentionally quite informal and subjective. Situations will come up that may not have been anticipated by this article, and they should be handled on a case-by-case basis.

If something is ambiguous in this article, that means it is left up to discussion. If something is a persistent cause of confusion, then it may be wise to amend the RFC process itself for clarification.

## License

The present document is adapted from the [SuperCollider RFC guide](https://github.com/supercollider/rfcs), and licensed under the terms of the [CC-BY-SA-4.0](https://creativecommons.org/licenses/by-sa/4.0/legalcode).
