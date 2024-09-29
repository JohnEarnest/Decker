#!/bin/sh

# install application icon
xdg-icon-resource install --novendor --context apps --size 32 icon_32x32.png decker
xdg-icon-resource install --novendor --context apps --size 64 icon_64x64.png decker
xdg-icon-resource install --novendor --context apps --size 128 icon_128x128.png decker
xdg-icon-resource install --novendor --context apps --size 192 icon_192x192.png decker
xdg-icon-resource install --novendor --context apps --size 256 icon_256x256.png decker
xdg-icon-resource install --novendor --context apps --size 512 icon_512x512.png decker

# install mime type icon
xdg-icon-resource install --context mimetypes --size 32 icon_32x32.png x-decker
xdg-icon-resource install --context mimetypes --size 64 icon_64x64.png x-decker
xdg-icon-resource install --context mimetypes --size 128 icon_128x128.png x-decker
xdg-icon-resource install --context mimetypes --size 192 icon_192x192.png x-decker
xdg-icon-resource install --context mimetypes --size 256 icon_256x256.png x-decker
xdg-icon-resource install --context mimetypes --size 512 icon_512x512.png x-decker

# register mime type
xdg-mime install x-decker.xml

# register application launcher
xdg-desktop-menu install --novendor Decker.desktop
