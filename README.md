# panda_prosthesis (Rolkneematics)

This repository gathers controllers centered around the development and evaluation of Orthosis medical devices.
It contains two controllers:
- `PandaProsthesis` is a controller dedicated to the evaluation of BoneTag's capacitive sensors. The aim is to provide non-invasive imaging of in-patient knee prosthesis.
- `PandaBrace` is a controller dedicated to the evaluation of experiemental medical Orthosis devices. The aim is to use a robotics setup to quantify how an Orhosis reduces the applied forces on the knee joints.

## PandaProsthesis (BoneTag demo)

![PandaProsthesis image](doc/images/PandaProsthesis.png)

For details on how to use the `PandaProsthesis` controller, see [PandaProsthesis documentation](doc/PandaProsthesis.md)

## PandaBrace (Lea's Orthesis demo)

![PandaBrace image](doc/images/PandaBrace.png)

For details on how to use the `PandaBrace` controller, see [PandaBrace documentation](doc/PandaBrace.md)

## Building
### With Nix (recommanded)

#### Setup Nix

If you are here and don't have nix yet, here is probably the easiest and fastest way to get started on ubuntu >= 24.04 "noble" / debian >= 13 "trixie" (because we need nix >= 2.18):

```sh
# 1. install the right apt package
sudo apt install -y nix-setup-systemd

# 2. activate the new CLI and flake features
echo 'experimental-features = nix-command flakes' | sudo tee -a /etc/nix/nix.conf

# 3. (optional) if you trust us, add our binary caches to avoid recompiling everything
echo 'extra-substituters = https://gepetto.cachix.org https://attic.iid.ciirc.cvut.cz/ros https://mc-rtc-nix.cachix.org' | sudo tee -a /etc/nix/nix.conf
echo 'extra-trusted-public-keys = gepetto.cachix.org-1:toswMl31VewC0jGkN6+gOelO2Yom0SOHzPwJMY2XiDY= ros:JR95vUYsShSqfA1VTYoFt1Nz6uXasm5QrcOsGry9f6Q= mc-rtc-nix.cachix.org-1:5M3sLvHXJCep4wc1tQl7QuFWL2eH2I0jkuvWtqJDYQs=' | sudo tee -a /etc/nix/nix.conf

# 4. activate your new nix.conf
sudo systemctl restart nix-daemon

# 5. allow yourself to use nix
sudo usermod -aG nix-users $(whoami)
newgrp nix-users

# 6. test everything is fine
nix run nixpkgs#ponysay it works
```

#### Run the project

To simply run the project, use

```sh
nix develop .#panda-prosthesis-full
```

you can test the project with

```sh
(mc-rtc-magnum &) # run visualization in the background
mc_rtc_ticker # run open-loop simulation
# or
MCFrankaControl # run on the real robot (you need network configuration and to setup the robot)
```

#### Develop the project

To develop the project (build from source), use
```sh
nix develop .#panda-prosthesis-full-devel
# you are now in a shell with all project dependencies installed, but not the project itself
cmake -B build $cmakeFlags -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -G Ninja
cmake --build build --target install
```

now the project is built and install, run as before (this will use the version you just compiled):

```sh
(mc-rtc-magnum &) # run visualization in the background
mc_rtc_ticker # run open-loop simulation
# or
MCFrankaControl # run on the real robot (you need network configuration and to setup the robot)
```

If you wish to make this automatic in the future, activate direnv with

```sh
direnv allow
```

From now on simply doing `cd panda_prosthesis` to enter this directory will enter the environnement and you can build, install and execute the project as usual.

### or... with superbuild

You can add the [panda-prosthesis-superbuild](https://github.com/arntanguy/panda-prosthesis-superbuild) to your superbuild extension folder:

The following options are available:
- `WITH_PANDA_BRACE:BOOL`: Whether to build the PandaBrace controller and its related dependencies
- `WITH_MC_RTC_ATI_DAQ:BOOL`: [optional in simulation/required for real experiments] Whether to build the `AtiDaq` plugin. This plugin allows to acquire ATI force sensor data plugged to a data acquisition card.
- `WITH_PHIDGET_PRESSURE_SENSOR_PLUGIN:BOOL`: [optional in simulation/required for real experiments] Whether to build the `PhidgetPressureSensor` plugin. This plugin is used to read pressure data from the Orthesis
- `WITH_TACTILE_FORCE_SENSOR_PLUGIN:BOOL`: [optional] Whether to build the `TactileForceSensor` plugin. This allows reading of pressure sensors placed on the surface of the tibia joint.

```sh
cd mc-rtc-superbuild/extensions
git clone https://github.com/arntanguy/panda-prosthesis-superbuild.git
cd ../build
make # clone and build dependencies with superbuild
```
