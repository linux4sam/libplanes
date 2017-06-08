# Plane Engine Config File Format

The config file is a strict [JSON][1] fomatted file. This data is loaded to configure
planes and optionally engine parameters. Unknown names will be ignored.

[1]: http://www.json.org/

## Integers
Integers can be specified as a literal integer (255), a hexadecimal integer
("0xFF"), and also in some cases as a relative percent ("50%").

## Colors
All colors in the config file, irrelevant of the actual plane format, use a
simple RGBA integer format. Of course, the alpha component is only actually used
if the plane format supports it.

The format of the integer is essentially "0xRRGGBBAA".  For example,
"0xFF0000FF" is an pure red color, "0x00FF00FF" is a pure blue color, and so on.

## Options

### root:framedelay
Number of milliseconds to delay for each frame.
* Type: Integer
* Example: `"framedelay": 10`

### root:planes[]
An array of planes.  There are two types of planes: primary and overlay. The
available options differ between these two plane types as appropriate.
* Type: Array of Objects
* Example:
```
{
    "type": "primary",
    "pattern": [ "0xffffffff", "0xD3D3D3ff" ],
    "format": "DRM_FORMAT_ARGB8888",
    "text": {
        "str": "Primary 0",
        "x": 10,
        "y": 460,
        "color": "0x000000ff"
    }
},
{
    "type": "overlay",
    "index": 0,
    "x": 90,
    "y": 140,
    "width": 200,
    "height": 200,
    "pattern": [ "0x0000ffff", "0x0000ffff" ],
    "format": "DRM_FORMAT_ARGB8888",
    "text": {
        "str": "Overlay 0",
        "x": 10,
        "y": 30,
        "color": "0x000000ff"
    }
}
```

#### root:planes[]:type
The type of plane.
* Type: String
  * "overlay"
  * "primary"
* Example: `"type": "primary"`

#### root:planes[]:enabled
Planes are enabled by default.  This can be used to disable a plane configuration.
* Type: Bool
  * true
  * false
* Example: `"enabled": false`

#### root:planes[]:index
The index of plane based on type, starting at zero.
* Type: Integer
* Example: `"index": 0`

#### root:planes[]:x
The X coordinate of the plane.
* Type: Integer
* Example: `"x": 0`

#### root:planes[]:y
The Y coordinate of the plane.
* Type: Integer
* Example: `"y": 0`

#### root:planes[]:width
The width of the plane.
* Type: Integer
* Example: `"width": 100`
* Example: `"width": 10%`

#### root:planes[]:height
The height of the plane.
* Type: Integer
* Example: `"height": 100`
* Example: `"height": 10%`

#### root:planes[]:alpha
The alpha value of the plane.
* Type: Integer
* Example: `"alpha": "0xaa"`

#### root:planes[]:format
The DRM format of the plane.  This is a string representation of any format in
libdrm `<drm_fourcc.h>`.
* Type: String
* Example: `"format": "DRM_FORMAT_ARGB8888"`

#### root:planes[]:scale
A floating point value for the scale of the plane, where 1.0 is unscaled.
* Type: Double
* Example: `"scale": 0.5`

#### root:planes[]:rotate
Rotation degrees of the plane.
* Type: Integer
* Example: `"rotate": 90`

#### root:planes[]:image
Path to a PNG image file that should be loaded into the frame.  Image paths are
relative to the path of the config file they are in.
* Type: String
* Example: `"image": "image.png"`

#### root:planes[]:pattern
An array of 1 or 2 color values used to fill the entire plane.  If 2 color
values are used, a checker board pattern will be drawn.
* Type: Array of Integers
* Example: `"pattern": [ "0x00ff00ff", "0x0000ffff" ]`

#### root:planes[]:vgradient
An array of two colors used to draw a vertical gradient on the entire plane.
* Type: Array of Integers
* Example: `"vgradient": [ "0xff00ffff", "0x0000ffff" ]`

#### root:planes[]:move-type
An array of move types.
* Type: Array of Strings
  * "x-warp"
  * "x-bounce"
  * "x-bounce-custom"
  * "x-bounce-out"
  * "y-warp"
  * "y-bounce"
  * "y-bounce-custom"
  * "y-bounce-out"
  * "panx-bounce"
  * "pany-bounce"
  * "scaler"
  * "sprite"
* Example: `"move-type": [ "x-bounce", "y-bounce" ]`

#### root:planes[]:transform
An array of transform types.
* Type: Array of Strings
  * "flip-horizontal"
  * "flip-vertical"
  * "rotate-clockwise"
  * "rotate-cclockwise"
* Example: `"transform": [ "flip-horizontal" ]`

#### root:planes[]:move-xspeed
The speed at which the X coordinate value should be incremented each frame.
Positive or negative.
* Type: Integer
* Example: `"move-xspeed": 2`

#### root:planes[]:move-yspeed
The speed at which the Y coordinate value should be incremented each frame.
Positive or negative.
* Type: Integer
* Example: `"move-yspeed": -2`

#### root:planes[]:move-xmin
For `x-bounce-custom`, the minimum X coordinate value.
Positive or negative.
* Type: Integer
* Example: `"move-xmin": 100`

#### root:planes[]:move-xmax
For `x-bounce-custom`, the maximum X coordinate value.
Positive or negative.
* Type: Integer
* Example: `"move-xmax": 500`

#### root:planes[]:move-ymin
For `y-bounce-custom`, the minimum Y coordinate value.
Positive or negative.
* Type: Integer
* Example: `"move-ymin": 100`

#### root:planes[]:move-ymax
For `y-bounce-custom`, the maximum Y coordinate value.
Positive or negative.
* Type: Integer
* Example: `"move-xmax": 100`

#### root:planes[]:pan-x
The X coordinate of the pan.
* Type: Integer
* Example: `"pan-x": 10`

#### root:planes[]:pan-y
The X coordinate of the pan.
* Type: Integer
* Example: `"pan-y": 10`

#### root:planes[]:pan-width
* Type: Integer
* Example: `"pan-width": 100`

#### root:planes[]:pan-height
* Type: Integer
* Example: `"pan-height": 100`

#### root:planes[]:move-panyspeed
* Type: Integer
* Example: `"move-panyspeed": 2`

#### root:planes[]:move-panxspeed
* Type: Integer
* Example: `"move-panxspeed": 2`

#### root:planes[]:scaler-min
* Type: Double
* Example: `"scaler-min": 0.5`

#### root:planes[]:scaler-max
* Type: Double
* Example: `"scaler-max": 2.0`

#### root:planes[]:scaler-speed
* Type: Double
* Example: `"scaler-speed": 0.1`

#### root:planes[]:sprite-x
The current position of the sprite frame. This should almost always start at 0.
* Type: Integer
* Example: `"sprite-x": 0`

#### root:planes[]:sprite-y
The current position of the sprite frame. This should almost always start at 0.
* Type: Integer
* Example: `"sprite-y": 0`

#### root:planes[]:sprite-width
The width of a sprite frame.
* Type: Integer
* Example: `"sprite-width": 100`

#### root:planes[]:sprite-height
The height of a sprite frame.
* Type: Integer
* Example: `"sprite-height": 100`

#### root:planes[]:sprite-count
The number of sprite frames in the image. They may be all horizontal, all
vertical, or a mixture of both.
* Type: Integer
* Example: `"sprite-count": 18`

#### root:planes[]:sprite-speed
The number of frames required to advance the sprite frame.  This can be used to
control the speed of the sprite relative to frame rate.
* Type: Integer
* Example: `"sprite-speed": 5`

#### root:planes[]:text
Render some text to the plane.
* Type: Object
* Example:
```
"text": {
    "str": "Hello World",
    "x": 50,
    "y": 50,
    "color": "0x000000ff"
}
```

#### root:planes[]:text:str
The text string.
* Type: String
* Example: `"str": "Hello World"`

#### root:planes[]:text:x
The X position of the text string.  Note that cairo is used to render text and
this position is relative to the bottom left of the text.
* Type: Integer
* Example: `"x": 50`

#### root:planes[]:text:y
The X position of the text string.  Note that cairo is used to render text and
this position is relative to the bottom left of the text.
* Type: Integer
* Example: `"y": 50`

#### root:planes[]:text:color
The foreground color of the text.
* Type: Integer
* Example: `"color": "0x000000ff"`
