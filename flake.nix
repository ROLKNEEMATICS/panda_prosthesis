{
  description = "PandaProsthesis controller for the Rolkneematics project";

  inputs = {
    # mc-rtc-nix.url = "github:mc-rtc/nixpkgs";
    mc-rtc-nix.url = "path:/home/arnaud/devel/mc-rtc-nix/nixpkgs";
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
              ];
              extraDevPackages = [ "pkg-config" ];
              overrideAttrs.mc-panda =
                _:
                (_super: {
                  src = lib.cleanSource /home/arnaud/devel/mc-rtc-nix/workspace/mc_panda;
                  # cmakeFlags = super.cmakeFlags ++ [
                  #   "-DPYTHON_BINDINGS=OFF"
                  # ];
                });
              overrideAttrs.mc-panda-lirmm =
                _:
                (_super: {
                  src = lib.cleanSource /home/arnaud/devel/mc-rtc-nix/workspace/mc_panda_lirmm;
                });
              overrideAttrs.panda-prosthesis =
                _:
                (_super: {
                  src = lib.cleanSource ./.;
                });
              overrideAttrs.mc-rtc-superbuild =
                final:
                (super: {
                  robots = [
                    final.panda-prosthesis
                    final.mc-panda-lirmm
                    final.mc-panda
                  ];
                  controllers = [ final.panda-prosthesis ];
                  # extra mc_rtc.yaml
                  configs = [ "${final.panda-prosthesis}/lib/mc_controller/etc/mc_rtc.yaml" ];
                  observers = [];
                  plugins = [ final.panda-prosthesis ];
                  apps = [ final.mc-rtc-magnum final.mc-franka final.mc-rtc-ticker ];
                });
            };
          }
        ];
      }
    );
}
