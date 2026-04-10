{
  description = "PandaProsthesis controller for the Rolkneematics project";

  inputs = {
    # mc-rtc-nix.url = "github:mc-rtc/nixpkgs";
    # mc-rtc-nix.url = "path:/home/arnaud/devel/mc-rtc-nix/nixpkgs";
    mc-rtc-nix.url = "github:arntanguy/nixpkgs-1?ref=topic/flakoboros";
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
              overrideAttrs.panda-prosthesis =
                _:
                (_super: {
                  src = lib.cleanSource ./.;
                  # cmakeFlags = super.cmakeFlags ++ [
                  #   "-DPYTHON_BINDINGS=OFF"
                  # ];
                });
            };
          }
        ];
      }
    );
}
