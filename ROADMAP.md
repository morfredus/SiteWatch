# Roadmap — SiteWatch

This roadmap describes possible directions for SiteWatch. It is not a promise of
dates: it exists to keep a clear direction for the next versions.

## Vision

SiteWatch should help you quickly decide where to intervene.

The goal is not to add ever more statistics, but to make the important signals
immediately visible: server errors, attacks, bots, unusual WordPress activity,
Google behavior and URL problems.

## Short term

- Improve the **Sites** tab as the main dashboard after a synchronization.
- Add an explicit global synchronization for all configured sites.
- Make the points of attention more precise without introducing AI: simple,
  understandable and verifiable rules.
- Add more in-app help messages when no log or no data is available.

## Medium term

- Compare a period with the previous one, for example the last 30 days against
  the 30 days before.
- Show significant variations: humans, AI bots, attacks, 404 errors, 500 errors,
  Google activity.
- Show a small history of a site's state over the last synchronizations.
- Keep health summaries to track a site's evolution over time.

## Possible alerts

- New site in a red state.
- Sharp rise in attacks.
- Surge of 404 errors.
- Significant drop in Google activity.
- Unusual increase in AI bots.
- Abnormal WordPress activity.

## Operation and maintenance

- Better document non-o2switch hosting cases.
- Make it easier to export the data useful for diagnostics.
- Keep simplifying the Windows build around MSYS2/MinGW.
- Maintain the Linux deployment (AppImage + desktop integration) and keep it up
  to date with new Qt versions.
- Keep an architecture where health rules are centralized and reusable by the
  Health tab, the Sites tab and future alerts.

## Out of scope

SiteWatch is not meant to replace a marketing audience-measurement tool. The
priority remains administration, technical monitoring and understanding the
events actually present in the server logs.
