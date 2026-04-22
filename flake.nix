{
  description = "PandaProsthesis controller for the Rolkneematics project";

  inputs = {
    mc-rtc-nix.url = "github:mc-rtc/nixpkgs";
    # mc-rtc-nix.url = "path:/home/arnaud/devel/mc-rtc-nix/nixpkgs";
    # mc-rtc-nix.url = "github:arntanguy/nixpkgs-1?ref=topic/flakoboros";
    flake-parts.follows = "mc-rtc-nix/flake-parts";
    systems.follows = "mc-rtc-nix/systems";

    mc-panda.url = "github:jrl-umi3218/mc_panda/pull/17/head";
    mc-panda-lirmm.url = "github:jrl-umi3218/mc_panda_lirmm/pull/16/head";
    mc-panda-lirmm.flake = false;
  };

  outputs =
    inputs:
    inputs.flake-parts.lib.mkFlake { inherit inputs; } (
      { lib, ... }:
      {
        systems = import inputs.systems;
        imports = [
          inputs.mc-rtc-nix.flakeModule
          {
            flakoboros = {
              extraPackages = [
                "ninja"
                # FIXME: why are these needed here?
                # "pkg-config"
                # "rosidl-default-generators"
                # # "geometry-msgs"
                # "rosidl-default-runtime"
                # "rosidl-typesupport-c"
                # "rosidl-typesupport-cpp"
                # "ament-cmake"
                # "mc-rtc-magnum"
              ];
              extraDevPackages = [ "pkg-config" "fmt" ];
              overrideAttrs.mc-panda = {
                src = inputs.mc-panda;
              };
              overrideAttrs.mc-panda-lirmm = {
                src = inputs.mc-panda-lirmm;
              };

              overrideAttrs.panda-prosthesis = {
                src = lib.cleanSource ./.;
              };

              # overrideAttrs.mc-rtc =
              # { pkgs-prev, pkgs-final, drv-prev, ... }:
              # {
              #   propagateBuildInputs = drv-prev.propagatedBuildInputs ++ [ pkgs-final.fmt ];
              # };

              # overrides override package function arguments, while overrideAttrs overrides the attribute set
              overrides.mc-rtc-superbuild =
                { pkgs-final, pkgs-prev, ... }:
                let
                  cfg-prev = pkgs-prev.mc-rtc-superbuild.superbuildArgs;
                in
                {
                  superbuildArgs = cfg-prev //
                  {
                    pname = "panda-prosthesis-superbuild";
                    traceRuntimeDependencies = true;
                    robots = [
                      pkgs-final.panda-prosthesis
                      pkgs-final.mc-panda-lirmm
                      pkgs-final.mc-panda
                    ];
                    controllers = [ pkgs-final.panda-prosthesis ];
                    # extra mc_rtc.yaml
                    configs = [ "${pkgs-final.panda-prosthesis}/lib/mc_controller/etc/panda_prosthesis/mc_rtc.yaml" ];
                    plugins = [ pkgs-final.panda-prosthesis ];
                    apps = cfg-prev.apps ++ [
                      pkgs-final.mc-franka
                    ];
                  };
                };
            };
          }
        ];
        perSystem =
          { pkgs, ... }:
          {
            packages.default = pkgs.panda-prosthesis;
            devShells.default = pkgs.callPackage "${inputs.mc-rtc-nix}/shell.nix" {
              inherit pkgs;
              mc-rtc-superbuild = pkgs.mc-rtc-superbuild;
            };
          };
      }
    );
}
