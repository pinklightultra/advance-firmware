<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright (c) 2026 xiphonics, inc.
-->

# Licensing

This repository contains material under several licenses. The default license
for new original project contributions is the GNU General Public License,
version 3 only (`GPL-3.0-only`), unless explicitly stated otherwise. This default
does not replace the licenses of inherited or imported material.

## BSD baseline

The baseline is the public [picoTracker v2.3.0 release](https://github.com/xiphonics/picoTracker/releases/tag/v2.3.0),
at commit `10ebbe0473ebdfa34f7456bbaf213ad83bb4540b`.
Its project code and documentation retain their BSD-3-Clause license, except
where separate third-party terms apply. The original BSD license, copyright
notices, conditions, and disclaimer are preserved in
[LICENSES/BSD-3-Clause.txt](LICENSES/BSD-3-Clause.txt).

Previously granted BSD permissions remain available for material released under
BSD, including material imported from later BSD releases or other BSD projects.
A new file path, a later import date, or a changed license notice does not erase
that provenance or those permissions.

## New contributions and mixed-license files

New original contributions by xiphonics, inc. after the baseline are licensed
under `GPL-3.0-only` unless explicitly stated otherwise. Other contributions
must have permission from their copyright holders for the license stated in
their notices. Third-party material retains its own license.

An original file containing only GPL-licensed contributions uses
`SPDX-License-Identifier: GPL-3.0-only`.

An existing BSD file containing new GPL-licensed contributions uses
`SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only`, together with a notice
explaining the scope of each license. BSD applies to the inherited portions;
GPLv3 applies to the GPL-licensed contributions. Distributing the combined file
requires compliance with both licenses. This is not a choice to distribute the
new GPL contributions under BSD instead.

Existing copyright notices, license conditions, and disclaimers must be
preserved. The full GPLv3 text is in
[LICENSES/GPL-3.0-only.txt](LICENSES/GPL-3.0-only.txt). The `-only` identifier
specifies version 3; it does not grant an option to choose a later GPL version.

## Third-party material

The existing component inventory is preserved in [LICENSE](LICENSE).
Individual component and file notices determine the applicable terms; a
directory-level summary does not override a different notice within that
directory. Imported code, fonts, and other assets are not automatically
relicensed by this transition.

The inventory is not a complete compatibility audit. Before distributing a
combined GPL firmware release, verify the terms of every included component,
including embedded font assets and files with notices that differ from their
component's top-level license.
