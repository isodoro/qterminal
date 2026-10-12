# QTerminal – Kali Enhanced

An independently maintained Kali Linux enhancement of **QTerminal 2.4.0**, based on the original LXQt project.

This fork adds terminal-management features designed to improve productivity when working with multiple terminal tabs and panes.

## Features

### Variable Session Manager (V.S.M.)

Per-terminal-pane management of shell variables in interactive Zsh sessions.

- Create, edit, list and delete managed variables.
- Maintain independent variable values in each terminal pane.
- Reuse the same variable names across different panes without conflicts.
- Automatically refresh the variable list when changes are made.
- Integrates with Zsh automatically when QTerminal starts an interactive Zsh session.

### Improved Bookmark Filtering

Enhanced bookmark search and filtering:

- Search bookmark names and command contents.
- Match items inside nested bookmark groups.
- Keep parent groups visible when matching items are found.
- Automatically expand matching groups and their parent paths.
- Restore the complete bookmark tree when the search is cleared.

## Demonstrations

The following videos demonstrate the additional functionality introduced by this fork.

### Variable Session Manager (V.S.M.)

- [Active Pane Display](docs/videos/active-pane-display.mp4) – Displaying the active terminal pane value change.
- [LEAD Descriptions](docs/videos/lead-descriptions.mp4) – LEAD Descriptions – Demonstrating descriptive labels and information associated with managed variables.
- [V.S.M. and LEAD Integration](docs/videos/vars-leads-test.mp4) – Demonstrating the interaction between the Variable Session Manager and LEAD functionality.

### Improved Bookmark Filtering

- [Improved Filtering](docs/videos/improved-filtering.mp4) – Searching bookmark names and command contents, including nested groups.
- [Expand and Collapse](docs/videos/expand-and-collapse.mp4) – Demonstrating bookmark group expansion and collapse behaviour.

## Platform and Compatibility

This project is developed specifically for **Kali Linux**.

- **Tested on:** Kali Linux 2026.1 (fresh installation).
- **Package format:** Debian `.deb` (amd64).
- **Shell integration:** Interactive Zsh sessions.
- **Other distributions:** Not currently tested or supported.

This is an independent downstream build, not a general-purpose replacement for upstream QTerminal across other Linux distributions.

## Installation

Download the Debian package from the project's [GitHub Releases](https://github.com/isodoro/qterminal/releases/latest) page.

Install the downloaded package using:

```bash
sudo apt install ./qterminal_2.4.0-1+isodoro1_amd64.deb
```

APT will resolve the package's required dependencies.

To launch QTerminal:

```bash
qterminal
```

## Building from Source

Clone the project source:

```bash
git clone --branch kali-vsm https://github.com/isodoro/qterminal.git
cd qterminal
```
The project uses the standard Debian packaging build process.

Install the required build dependencies:

```bash
sudo apt install build-essential debhelper cmake pkgconf \
    qt6-base-dev qt6-tools-dev liblayershellqtinterface-dev \
    libqtermwidget-dev lxqt-build-tools libx11-dev
```

Build the Debian package:

```bash
dpkg-buildpackage -us -uc -b
```

The resulting Debian packages are created in the parent directory of the source tree.

## Upstream Project and Attribution

This project is based on **QTerminal 2.4.0**, maintained by the LXQt team.

- Original project: https://github.com/lxqt/qterminal
- Development fork: https://github.com/isodoro/qterminal

QTerminal is licensed under GPL-2.0-or-later. Third-party components retain their applicable licences and copyright notices.

This is an independently maintained downstream project and is not an official LXQt or Kali Linux release.

## Contributing

Development and issue tracking take place in this repository.

Feedback, suggestions and testing reports are welcome.
