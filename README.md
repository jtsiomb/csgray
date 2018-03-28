Simple CSG raytracer
====================

Just a simple CSG (Constructive Solid Geometry) raytracer. Takes text-based
scene descriptions as input, and produces an image.

License
-------
Copyright (C) 2018 John Tsiombikas <nuclear@member.fsf.org>
This program is free software. You may use, modify, and/or redistribute it under
the terms of the GNU General Public License v3, or at your option any later
version published by the Free Software Foundation. See COPYING for details.

Build
-----
Just type make to build. You'll need `libtreestore` from
http://github.com/jtsiomb/libtreestore

To build the interactive variant `xcsgray`, change into xcsgray and type make.
You'll need `freeglut`, and `libresman` which is used for automatic modification
tracking and reloading of the scene description file. You can find it here:
http://github.com/jtsiomb/libresman
