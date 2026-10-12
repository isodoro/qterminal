# QTerminal – Kali Enhanced

An independently maintained Kali Linux enhancement of **QTerminal**, based on the upstream QTerminal 2.4.0 release.

This project introduces additional terminal-management features designed to improve productivity when working with multiple terminal tabs and panes.

## Features

### Variable Session Manager (V.S.M.)

V.S.M. provides per-terminal-pane shell variable management.

- Create, edit, list and delete managed variables.
- Maintain independent variable values in each terminal pane.
- Use the same variable names across different terminal sessions without conflicts.
- Automatically refresh the variable list when changes are made.

The initial implementation targets interactive Zsh sessions.

### Improved Bookmark Filtering

Enhanced bookmark search and filtering:

- Search bookmark names and command contents.
- Match nested bookmark groups.
- Keep parent groups visible when matching items are found.
- Automatically expand matching groups and their relevant parent paths.
- Restore the complete bookmark tree when the search is cleared.

## Platform

This project is specifically developed and tested for **Kali Linux**.

It is not intended to be a general-purpose replacement for upstream QTerminal across multiple Linux distributions.

## Project Status

Development in progress.

Current development priorities:

- Complete automatic V.S.M. integration with interactive Zsh sessions.
- Finalise Debian package creation and installation.
- Test installation and operation on a fresh Kali Linux system.
- Prepare installation and build documentation.

## Installation

Installation packages are not yet available.

The intended distribution method is a downloadable Debian `.deb` package, with instructions for users who prefer to build from source.

## Building

Build instructions will be provided once the Debian packaging and installation process have been finalised.

## Upstream Project and Attribution
The original QTerminal project is licensed under GPL-2.0-or-later. See the LICENSE file for the applicable licence terms.

This project is based on the original QTerminal project maintained by the LXQt team.

- Original project: https://github.com/lxqt/qterminal
- Original version: 2.4.0
- Development fork: https://github.com/isodoro/qterminal

All original QTerminal components remain subject to their applicable upstream licensing and copyright notices.

This is an independent project and is not an official LXQt or Kali Linux release.

## Contributing

Development and issue tracking take place in this repository.

Feedback, suggestions and testing reports are welcome.
