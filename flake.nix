{
  description = "PandaProsthesis controller for the Rolkneematics project";

  inputs = {
    mc-rtc-nix.url = "github:mc-rtc/nixpkgs";
    flake-parts.follows = "mc-rtc-nix/flake-parts";
    systems.follows = "mc-rtc-nix/systems";
    gepetto.follows = "mc-rtc-nix/gepetto";
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
