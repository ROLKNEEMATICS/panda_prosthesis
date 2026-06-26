{
  description = "PandaProsthesis controller for the Rolkneematics project";

  inputs = {
    # mc-rtc-nix.url = "github:mc-rtc/nixpkgs";
    mc-rtc-nix.url = "github:mc-rtc/nixpkgs/pull/65/head";
    # mc-rtc-nix.url = "path:/home/arnaud/devel/mc-rtc-nix/nixpkgs";
    flake-parts.follows = "mc-rtc-nix/flake-parts";
    systems.follows = "mc-rtc-nix/systems";
    gepetto.follows = "mc-rtc-nix/gepetto";

    mc-panda.url = "github:jrl-umi3218/mc_panda/pull/17/head";
    mc-panda-lirmm.url = "github:jrl-umi3218/mc_panda_lirmm/pull/16/head";
    mc-panda-lirmm.flake = false;

    # FIXME: for USE_REALTIME=false
    # mc-franka.url = "github:jrl-umi3218/mc_franka/pull/16/head";
    mc-franka.url = "github:arntanguy/mc_franka/563e4fbf3383977568f0246360ec78fc8bdf2c77";
  };

  nixConfig = {
    extra-substituters = [
      "https://mc-rtc-nix.cachix.org"
      "https://gepetto.cachix.org"
      "https://attic.iid.ciirc.cvut.cz/ros"
    ];
    extra-trusted-public-keys = [
      "mc-rtc-nix.cachix.org-1:5M3sLvHXJCep4wc1tQl7QuFWL2eH2I0jkuvWtqJDYQs="
      "gepetto.cachix.org-1:toswMl31VewC0jGkN6+gOelO2Yom0SOHzPwJMY2XiDY="
      "ros:JR95vUYsShSqfA1VTYoFt1Nz6uXasm5QrcOsGry9f6Q="
    ];
  };

  outputs =
    inputs:
    inputs.flake-parts.lib.mkFlake { inherit inputs; } (
      { lib, ... }:
      {
        systems = [ "x86_64-linux" ];
        imports = [
          inputs.mc-rtc-nix.flakeModule
          {
            # mc-rtc-nix.with-ros = false;
            mc-rtc-superbuild =
              { pkgs, ... }:
              {
                enable = true;
                project.pname = "";
                configurations = {
                  panda-prosthesis-minimal = {
                    extends = [ "minimal" ];
                    runtime = {
                      robots = [
                        pkgs.mc-panda-lirmm
                        pkgs.mc-panda
                      ];

                      apps = [
                        pkgs.mc-rtc-magnum
                      ];
                      config = "lib/mc_controller/etc/panda_prosthesis/mc_rtc.yaml";
                    };
                    devel = {
                      config = "lib64/mc_controller/etc/panda_prosthesis/mc_rtc.yaml";
                      controllers = [ pkgs.panda-prosthesis ];
                      plugins = [ pkgs.panda-prosthesis ];
                      robots = [ pkgs.panda-prosthesis ];
                    };
                  };
                  panda-prosthesis-full = {
                    extends = [
                      "default"
                      "panda-prosthesis-minimal"
                    ];
                    runtime = {
                      apps = [
                        pkgs.mc-franka
                      ];
                    };
                  };
                };
              };

            flakoboros = {
              overlays = [
                inputs.mc-franka.overlays.flakoboros
              ];
              overrideAttrs.mc-franka = {
                src = inputs.mc-franka;
              };
              overrideAttrs.mc-panda = {
                src = inputs.mc-panda;
              };
              overrideAttrs.mc-panda-lirmm = {
                src = inputs.mc-panda-lirmm;
              };

              overrideAttrs.panda-prosthesis =
                { drv-prev, pkgs-final, ... }:
                {
                  src = lib.cleanSource ./.;
                  nativeBuildInputs = drv-prev.nativeBuildInputs ++ [ pkgs-final.pkg-config ];
                };
            };
          }
        ];
      }
    );
}
