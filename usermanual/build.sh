#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
#
# Copyright (c) 2026 xiphonics, inc.
#
# Inherited BSD-licensed material remains BSD-3-Clause.
# New original contributions by xiphonics, inc. are GPL-3.0-only.
# Previously granted BSD permissions are preserved. See LICENSING.md.

# builds the html and pdf outputs for the advance edition user manual
set -e

# clear out any old generated content
rm -rf web output

# build the advance edition
bin/picosite -a static -s advance-edition/content -d pdfconfig.yaml

mkdir -p web/advance
mv output/* web/advance/

cp landing/* web/

# picosite writes the PDF as output/output.pdf, which the mv above placed at
# web/advance/output.pdf. Give it a meaningful name so CI can publish it as a
# downloadable build artifact. With set -e this mv also fails the build if the
# PDF was not generated, instead of producing an artifact-less run.
mv web/advance/output.pdf web/advance/picoTracker-user-manual.pdf
