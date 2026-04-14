{
  description = "PandaProsthesis controller for the Rolkneematics project";

  inputs = {
    mc-rtc-nix.url = "github:mc-rtc/nixpkgs";
    # mc-rtc-nix.url = "path:/home/arnaud/devel/mc-rtc-nix/nixpkgs";
    # mc-rtc-nix.url = "github:arntanguy/nixpkgs-1?ref=topic/flakoboros";
    flake-parts.follows = "mc-rtc-nix/flake-parts";
    systems.follows = "mc-rtc-nix/systems";
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
                "pkg-config"
                "rosidl-default-generators"
                # "geometry-msgs"
                "rosidl-default-runtime"
                "rosidl-typesupport-c"
                "rosidl-typesupport-cpp"
                "ament-cmake"
                "mc-rtc-magnum"
              ];
              extraDevPackages = [ "pkg-config" ];
              overrideAttrs.mc-panda = {
                src = lib.cleanSource /home/arnaud/devel/mc-rtc-nix/workspace/mc_panda;
                # cmakeFlags = drv-prev.cmakeFlags ++ [
                #   "-DPYTHON_BINDINGS=OFF"
                # ];
              };
              overrideAttrs.mc-panda-lirmm = {
                src = lib.cleanSource /home/arnaud/devel/mc-rtc-nix/workspace/mc_panda_lirmm;
              };
              overrideAttrs.panda-prosthesis = {
                src = lib.cleanSource ./.;
              };
              # overrides override package function arguments, while overrideAttrs overrides the attribute set
              overrides.mc-rtc-superbuild =
                { pkgs-final, ... }:
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
                  configs = [ "${pkgs-final.panda-prosthesis}/lib/mc_controller/etc/mc_rtc.yaml" ];
                  observers = [ ];
                  plugins = [ pkgs-final.panda-prosthesis ];
                  apps = [
                    pkgs-final.mc-rtc-magnum
                    pkgs-final.mc-franka
                    pkgs-final.mc-rtc-ticker
                  ];
                };
            };
          }
        ];
        perSystem =
          { pkgs, ... }:
          {
            devShells.panda-prosthesis-superbuild = pkgs.callPackage "${inputs.mc-rtc-nix}/shell.nix" {
              inherit pkgs;
              name = "panda-prosthesis-local";
              mc-rtc-superbuild = pkgs.mc-rtc-superbuild;
            };
          };
      }
    );
}
