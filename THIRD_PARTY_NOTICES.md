# Third-Party and Mixed-Origin Notices

The top-level `LICENSE` is a proprietary, all-rights-reserved notice that
applies only to contributor-owned original material. It does **not** relicense
third-party, instructor, course, reference, or vendor material.

## Apache-2.0 material

The following files carry their own Apache License 2.0 headers:

- `scripts/demo_inspection.py` — Copyright 2021 Samsung Research America
- `src/publisher_member_function.cpp` — Copyright 2016 Open Source Robotics Foundation, Inc.

Their preserved headers control. A copy of Apache-2.0 is provided at
`LICENSES/Apache-2.0.txt`.

## GPL-labelled material

`src/autonomous_robot_system_dt1.cpp` identifies V. Sieben and labels the file
“GNU GPLv3.” It is treated conservatively as GPL-3.0-only unless its rights
holder clarifies otherwise. A copy is provided at
`LICENSES/GPL-3.0-only.txt`. Related course/reference files may have separate
ownership and are not automatically covered by the project notice.

## Automatic Addison material

Several `launch/lab*.launch.py` files attribute Addison Sears-Collins and
Automatic Addison. Historical development material also references an
Automatic Addison quaternion helper. Those materials are excluded from the project's proprietary notice and
remain subject to their original owner's terms. The project captain has chosen
to include them in this source archive based on the project's publication
decision; this is not a representation that an open-source license was granted.

## Microchip/Atmel material

The reachable `contributors/daniel15navarro/rescue-detection` branch contains
Microchip/Atmel Software Framework files, including `firmware/**/src/ASF/**`,
`src/asf.h`, `src/config/conf_board.h`, and generated Atmel project metadata. Their embedded copyright and license
headers control, including any device-use or redistribution restrictions. They
are excluded from the project's proprietary notice.

## Course, instructor, and reference material

The history identifies starter, reference, or solution material supplied for
an educational project, including some launch files, robot models, maps,
worlds, RViz/configuration files, and navigation parameters. Rights in such material remain with their respective owners. The completed
project is included based on the project captain's publication decision.
Publication does not place that material under the proprietary project notice
or waive any surviving third-party restriction.

## Dependencies

ROS 2, Nav2, Gazebo, RViz, `robot_localization`, `nav2_simple_commander`,
`launch_ros`, `pyserial`, and other external dependencies are not bundled merely
because they are referenced. Each remains governed by its own license.

No project-wide relicensing of third-party material is intended. When a file
header conflicts with this notice, the file header controls for that file.
