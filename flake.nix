{
  description = "PandaProsthesis controller for the Rolkneematics project";

  inputs = {
    mc-rtc-nix.url = "github:mc-rtc/nixpkgs";
    flake-parts.follows = "mc-rtc-nix/flake-parts";
    systems.follows = "mc-rtc-nix/systems";
    gepetto.follows = "mc-rtc-nix/gepetto";

    ccache-trigger.follows = "mc-rtc-nix/ccache-trigger";
  };

  outputs =
    inputs:
    inputs.mc-rtc-nix.lib.mkMcRtcModule inputs (
      { lib, ... }:
      {
        mc-rtc-nix.overlays.ccache = inputs.ccache-trigger.value;
        # mc-rtc-nix.with-ros = false;
        mc-rtc-superbuild =
          { pkgs, ... }:
          {
            enable = true;
            project.pname = "";
            configurations = {
              panda-prosthesis = inputs.mc-rtc-nix.lib.mkControllerSuperbuild pkgs pkgs.panda-prosthesis { };
              panda-prosthesis-full = {
                extends = [ "panda-prosthesis" ];
                runtime = {
                  apps = [
                    pkgs.mc-franka
                  ];
                };
              };
            };
          };

        flakoboros = {
          overrideAttrs.panda-prosthesis = {
            src = lib.cleanSource ./.;
          };
        };
      }
    );
}
