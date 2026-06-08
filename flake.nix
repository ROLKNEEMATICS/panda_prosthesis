{
  description = "PandaProsthesis controller for the Rolkneematics project";

  inputs = {
    # mc-rtc-nix.url = "github:mc-rtc/nixpkgs";
    mc-rtc-nix.url = "github:mc-rtc/nixpkgs/pull/44/head"; # local devshell update
    # mc-rtc-nix.url = "path:/home/arnaud/devel/mc-rtc-nix/nixpkgs";
    flake-parts.follows = "mc-rtc-nix/flake-parts";
    systems.follows = "mc-rtc-nix/systems";

    mc-panda.url = "github:jrl-umi3218/mc_panda/pull/17/head";
    mc-panda-lirmm.url = "github:jrl-umi3218/mc_panda_lirmm/pull/16/head";
    mc-panda-lirmm.flake = false;
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
              extraDevPackages = [
                "pkg-config"
                "fmt"
              ];
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
        perSystem =
          { pkgs, ... }:
          let
            commonSuperbuildArgs = {
              pname = "panda-prosthesis-superbuild";
              traceRuntimeDependencies = true;
              robots = [
                pkgs.mc-panda-lirmm
                pkgs.mc-panda
              ];
              apps = with pkgs; [
                mc-franka
                mc-rtc-magnum
              ];
              controllers = [ ];
              configs = [ ];
              plugins = [ ];
            };
          in
          {
            packages.default = pkgs.panda-prosthesis;
            devShells.superbuild-official = pkgs.callPackage "${inputs.mc-rtc-nix}/shellv2.nix" {
              inherit pkgs lib;
              superbuildArgs = commonSuperbuildArgs // {
                pname = "panda-prosthesis-superbuild-official";
                controllers = [ pkgs.panda-prosthesis ];
                configs = [ "${pkgs.panda-prosthesis}/lib/mc_controller/etc/panda_prosthesis/mc_rtc.yaml" ];
                plugins = [ pkgs.panda-prosthesis ];
              };
            };

            devShells.default = pkgs.callPackage "${inputs.mc-rtc-nix}/shellv2.nix" {
              inherit pkgs lib;
              superbuildArgs = commonSuperbuildArgs // {
                pname = "panda-prosthesis-superbuild-local";
              };

              develSuperbuildArgs = {
                controllers = builtins.trace "set devel controllers" [ pkgs.panda-prosthesis ];
                configs = [ "lib64/mc_controller/etc/panda_prosthesis/mc_rtc.yaml" ];
                plugins = [ pkgs.panda-prosthesis ];
                robots = [ pkgs.panda-prosthesis ];
              };
            };
          };
      }
    );
}
