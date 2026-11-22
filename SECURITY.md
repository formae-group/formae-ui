# Security policy

## Supported versions

Security fixes are made on `main` and released as a patch of the latest minor version.

| Version | Supported |
|---|---|
| 0.1.x | Yes |

## Reporting a vulnerability

Do not open a public issue. Report it privately by opening a **confidential issue** at
https://gitlab.com/formae/formae-ui/-/issues/new (tick *This issue is confidential*), or by
messaging a maintainer of the Formae group on GitLab. Include the version or commit, what an
attacker can do, and the steps or input (for example a crafted logo PNG) that reproduce it.

You will get an answer within seven days. Once a fix is released, the issue is made public and the
reporter credited unless they ask otherwise.

## Scope

formae-ui draws a menu from data its game supplies. Its main attack surface is the PNG decoder
(stb_image) used for logos, which normally only sees files embedded at build time. Vulnerabilities
in Dear ImGui or SDL2 should be reported to those projects.
