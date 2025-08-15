### Files taken from upstream

From commit `40dca384e6e6ad6afe3ed859017733455ee0b3d7`.

Everything in `include/tint`

Everything in `src` except:
- `src/dawn`
- `*.bazel`
- `*.gn` and related like `src/externals.json`
- `*.tmpl`
- `src/tint/cmd`
- `src/tint/lang/glsl`
- `src/tint/lang/spirv/reader`
- `src/tint/lang/hlsl` (may need to be re-added if support for DirectX is added)
- `src/tint/lang/msl` (may need to be re-added if support for Metal is added)
- Misc files: `tint_*.py`, `tint.gni` and `tint.natvis`

#### Files modified

`CMakeLists.txt`: Everything not related to building tint was removed.
