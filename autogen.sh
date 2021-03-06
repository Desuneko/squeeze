#!/bin/sh
#
# $Id: autogen.sh 22391 2006-07-10 12:54:50Z benny $
#
# Copyright (c) 2002-2006
#         The Xfce development team. All rights reserved.
#
# Written for Xfce by Benedikt Meurer <benny@xfce.org>.
#
XDT_REQURED_VERSION="4.8.0"

(type xdt-autogen) >/dev/null 2>&1 || {
  cat >&2 <<EOF
autogen.sh: You don't seem to have the Xfce development tools (at least
            version $XDT_REQURED_VERSION) installed on your system, which
            are required to build this software.
            Please install the xfce4-dev-tools package first; it is available
            from http://www.xfce.org/.
EOF
  exit 1
}

XDT_AUTOGEN_REQUIRED_VERSION=$XDT_REQURED_VERSION exec xdt-autogen "$@"

# vi:set ts=2 sw=2 et ai:
