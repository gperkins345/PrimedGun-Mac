// NativeRuntimePrime2.inc — Metroid Prime 2 (Echoes, G2ME01) runtime.
// #included from NativeRuntime.cpp inside its anonymous namespace (shares the
// memory-access + pose helpers). Kept in its own file so Prime 1 upstream merges
// never conflict with Prime 2 code. Compiled out by PRIMEDGUN_DISABLE_PRIME2.

// ---------------------------------------------------------------------------
// Metroid Prime 2: Echoes (G2ME01) — Phase-2 "head-aim" slice.
//
// Define PRIMEDGUN_DISABLE_PRIME2 to compile a pure Prime 1 build: this block
// and every prime2:: dispatch site vanish, leaving PrimedGun byte-equivalent
// to the Prime-1-only product. Prime 2 support packages separately this way.
//
// Additive by design: everything above this block is the Prime 1 (GM8E01)
// runtime and stays untouched. Prime 2 gets its own address table and a small
// per-frame path: level camera pitch + HMD-relative locomotion + snap turn +
// frustum-cull defeat. No code caves and no scratch-arena writes — the Prime 1
// arenas at 0x80001C00/0x80002800 are not assumed free in Prime 2's memory
// map. Addresses come from PrimeHack's G2ME01 tables and the PrimeDecomp/
// echoes symbol map; see PRIME2-PORT.md at the workspace root.
// ---------------------------------------------------------------------------
namespace prime2
{
constexpr u32 STATE_MANAGER = 0x803DB6E0u;      // CStateManager global
constexpr u32 PLAYER_OFFSET = 0x14FCu;          // -> CPlayer* (heap object, NOT a singleton)
constexpr u32 CAMERA_MANAGER_OFFSET = 0x151Cu;  // -> CCameraManager*
constexpr u32 CAMERA_UID_OFFSET = 0x14u;        // u16 current-camera uid in CCameraManager
constexpr u32 OBJECT_LIST_OFFSET = 0x810u;      // same uid<<3 scheme as Prime 1
constexpr u32 WORLD_OFFSET = 0x1604u;           // -> CWorld*
constexpr u32 WORLD_LOAD_PHASE_OFFSET = 0x4u;
constexpr u32 WORLD_LOAD_PHASE_READY = 4u;      // GC Echoes: 4 = fully loaded
constexpr u32 MENU_STATE_OFFSET = 0x2470u;      // != 0 -> menu/transition, input disabled
constexpr u32 TRANSFORM_OFFSET = 0x24u;         // CTransform in CActor (player and cameras)
constexpr u32 VELOCITY_OFFSET = 0x1A8u;
constexpr u32 MOVE_STATE_OFFSET = 0x2D0u;       // 0 = on ground
constexpr u32 CAMERA_STATE_OFFSET = 0x388u;     // != 0 -> morphball / cinematic camera
constexpr u32 BALL_STATE_OFFSET = 0x38Cu;
constexpr u32 ORBIT_STATE_OFFSET = 0x3A4u;      // 0 = free, 5 = grapple, else locked-on
constexpr u32 ORBIT_STATE_GRAPPLE = 5u;
constexpr u32 PITCH_OFFSET = 0x604u;            // first-person pitch, float radians
constexpr u32 GUN_OFFSET = 0xEBCu;              // -> CPlayerGun* (vtable-scan verified, live)
constexpr u32 GAME_STATE_GLOBAL = 0x80418EB8u;  // gpGameState (.sbss, decomp symbol)
// CGameState+0x80 = embedded CGameOptions; +0x20 = int helmetAlpha (decomp headers).
constexpr u32 GAME_STATE_HELMET_ALPHA_OFFSET = 0xA0u;
constexpr u32 EXCEPTION_HOOK_ADDRESS = 0x80000048u;  // != 0 once the game has booted
constexpr u32 TEXT_SECTION_BEGIN = 0x80003840u;
constexpr u32 TEXT_SECTION_END = 0x803A5494u;

struct InstructionPatch
{
  u32 address;
  u32 replacement;
  u32 original = 0;
  bool captured = false;
};

// PrimeHack's G2ME01 pitch-control kill set. With these installed the game stops driving
// camera pitch, and the per-frame zero write below keeps the view level so the HMD supplies
// all pitch at the render layer (same net effect as Prime 1's pitch caves).
InstructionPatch s_pitch_patches[] = {
    {0x801AEF58u, 0x60000000u},  // nop  - first-person pitch control
    {0x801AF160u, 0x60000000u},  // nop  - terrain-based pitch
    {0x800129C8u, 0x4E800020u},  // blr  - camera/aim update stub
    {0x801B0248u, 0x48000078u},  // b +0x78  - skip camera pitch/aim block
    {0x801AF450u, 0x48000A34u},  // b +0xa34 - skip camera control block
};

// Frustum-cull test forced to pass so VR-repositioned draws survive wide FOV.
InstructionPatch s_culling_patches[] = {
    {0x802F84C0u, 0x38600001u},  // li r3,1
    {0x802F84C4u, 0x4E800020u},  // blr
};

bool s_was_active = false;
bool s_patch_addresses_logged = false;
float s_move_speed = 0.0f;
bool s_snap_ready = true;
u64 s_snap_cooldown_until_frame = 0;
u64 s_gameplay_log_frame = 0;
bool s_gun_hook_installed = false;
u32 s_gun_hook_original = 0;
u64 s_gun_hook_log_frame = 0;
bool s_pad_hook_installed = false;
u32 s_pad_hook_original = 0;

bool IsMetroidPrime2(const Core::CPUThreadGuard& guard)
{
  constexpr std::array<u8, 6> game_id = {'G', '2', 'M', 'E', '0', '1'};
  for (size_t i = 0; i < game_id.size(); ++i)
  {
    const auto byte = PowerPC::MMU::HostTryRead<u8>(guard, 0x80000000u + static_cast<u32>(i));
    if (!byte || byte->value != game_id[i])
      return false;
  }
  return true;
}

void Reset()
{
  s_was_active = false;
  s_patch_addresses_logged = false;
  s_move_speed = 0.0f;
  s_snap_ready = true;
  s_snap_cooldown_until_frame = 0;
  s_gun_hook_installed = false;
  s_gun_hook_original = 0;
  s_pad_hook_installed = false;
  s_pad_hook_original = 0;
  s_vr_menu_visible = false;
  for (InstructionPatch& patch : s_pitch_patches)
  {
    patch.captured = false;
    patch.original = 0;
  }
  for (InstructionPatch& patch : s_culling_patches)
  {
    patch.captured = false;
    patch.original = 0;
  }
}

void NoteInactive()
{
  if (s_was_active)
    Reset();
}

bool GameBooted(const Core::CPUThreadGuard& guard)
{
  u32 hook = 0;
  return TryReadU32(guard, EXCEPTION_HOOK_ADDRESS, &hook) && hook != 0;
}

bool ApplyPatch(const Core::CPUThreadGuard& guard, InstructionPatch* patch)
{
  if (patch->address < TEXT_SECTION_BEGIN || patch->address >= TEXT_SECTION_END)
    return false;
  u32 current = 0;
  if (!TryReadU32(guard, patch->address, &current) || current == patch->replacement)
    return false;
  if (!patch->captured)
  {
    patch->original = current;
    patch->captured = true;
  }
  return TryWriteInstruction(guard, patch->address, patch->replacement);
}

bool ApplyPatches(const Core::CPUThreadGuard& guard, const RuntimeSettings& settings)
{
  if (!settings.builtin_patches_enabled)
    return false;
  bool wrote = false;
  for (InstructionPatch& patch : s_pitch_patches)
    wrote = ApplyPatch(guard, &patch) || wrote;
  for (InstructionPatch& patch : s_culling_patches)
    wrote = ApplyPatch(guard, &patch) || wrote;
  if (wrote && RuntimeLoggingEnabled() && !s_patch_addresses_logged)
  {
    s_patch_addresses_logged = true;
    for (const InstructionPatch& patch : s_pitch_patches)
    {
      NOTICE_LOG_FMT(CORE, "PrimedGun prime2 pitch patch {:08X}: {:08X} -> {:08X}",
                     patch.address, patch.original, patch.replacement);
    }
    for (const InstructionPatch& patch : s_culling_patches)
    {
      NOTICE_LOG_FMT(CORE, "PrimedGun prime2 culling patch {:08X}: {:08X} -> {:08X}",
                     patch.address, patch.original, patch.replacement);
    }
  }
  return wrote;
}

void InvalidatePatchedICache(Core::System& system)
{
  auto& jit = system.GetJitInterface();
  for (const InstructionPatch& patch : s_pitch_patches)
    jit.InvalidateICache(patch.address & ~0xFFFu, 0x1000u, true);
  for (const InstructionPatch& patch : s_culling_patches)
    jit.InvalidateICache(patch.address & ~0xFFFu, 0x1000u, true);
}

// Same behavior as Prime 1's ApplyHelmetOpacityZero: while the menu's helmet-visor toggle
// is off, hold the game's helmet-alpha option at 0 (Echoes exposes it as a game option).
void ApplyHelmetOpacity(const Core::CPUThreadGuard& guard, const RuntimeSettings& settings)
{
  if (settings.visor_helmet_enabled)
    return;

  u32 game_state = 0;
  if (!TryReadU32(guard, GAME_STATE_GLOBAL, &game_state) || game_state < 0x80000000u)
    return;

  const u32 helmet_alpha_addr = game_state + GAME_STATE_HELMET_ALPHA_OFFSET;
  u32 current = 0;
  if (TryReadU32(guard, helmet_alpha_addr, &current) && current != 0u && current <= 0xFFu)
    TryWriteU32(guard, helmet_alpha_addr, 0u);
}

bool PlayerLooksValid(const Core::CPUThreadGuard& guard, u32 player)
{
  if (!PrimeGameObjectPointerLooksValid(player, PITCH_OFFSET + 4u))
    return false;

  u32 camera_state = 0xffffffffu;
  u32 ball_state = 0xffffffffu;
  u32 move_state = 0xffffffffu;
  u32 orbit_state = 0xffffffffu;
  float pitch = 0.0f;
  float vx = 0.0f;
  float vy = 0.0f;
  float vz = 0.0f;
  if (!TryReadU32(guard, player + CAMERA_STATE_OFFSET, &camera_state) ||
      !TryReadU32(guard, player + BALL_STATE_OFFSET, &ball_state) ||
      !TryReadU32(guard, player + MOVE_STATE_OFFSET, &move_state) ||
      !TryReadU32(guard, player + ORBIT_STATE_OFFSET, &orbit_state) ||
      !TryReadFloat(guard, player + PITCH_OFFSET, &pitch) ||
      !TryReadFloat(guard, player + VELOCITY_OFFSET + 0x00u, &vx) ||
      !TryReadFloat(guard, player + VELOCITY_OFFSET + 0x04u, &vy) ||
      !TryReadFloat(guard, player + VELOCITY_OFFSET + 0x08u, &vz))
  {
    return false;
  }

  return camera_state <= 8 && ball_state <= 8 && move_state <= 32 && orbit_state <= 16 &&
         std::isfinite(pitch) && std::fabs(pitch) <= 3.2f && std::isfinite(vx) &&
         std::isfinite(vy) && std::isfinite(vz);
}

// Loading/menu gates (PrimeHack's): exception hook set, world fully loaded, no menu, and a
// live player. CPlayer is heap-allocated per load in Echoes, so every frame revalidates the
// pointer instead of trusting it across transitions.
bool ResolveGameplayPlayer(const Core::CPUThreadGuard& guard, u32* player_out)
{
  if (!GameBooted(guard))
    return false;

  u32 world = 0;
  u32 load_phase = 0;
  u32 menu_state = 0;
  u32 player = 0;
  if (!TryReadU32(guard, STATE_MANAGER + WORLD_OFFSET, &world) || world < 0x80000000u ||
      !TryReadU32(guard, world + WORLD_LOAD_PHASE_OFFSET, &load_phase) ||
      load_phase != WORLD_LOAD_PHASE_READY ||
      !TryReadU32(guard, STATE_MANAGER + MENU_STATE_OFFSET, &menu_state) || menu_state != 0 ||
      !TryReadU32(guard, STATE_MANAGER + PLAYER_OFFSET, &player) ||
      !PlayerLooksValid(guard, player))
  {
    return false;
  }

  *player_out = player;
  return true;
}

bool PlayerIsFirstPerson(const Core::CPUThreadGuard& guard, u32 player)
{
  u32 camera_state = 0xffffffffu;
  u32 ball_state = 0xffffffffu;
  return TryReadU32(guard, player + CAMERA_STATE_OFFSET, &camera_state) && camera_state == 0 &&
         TryReadU32(guard, player + BALL_STATE_OFFSET, &ball_state) && ball_state == 0;
}

float GetYawDeltaDegrees(const Core::CPUThreadGuard& guard)
{
  u32 camera_manager = 0;
  u32 object_list = 0;
  if (!TryReadU32(guard, STATE_MANAGER + CAMERA_MANAGER_OFFSET, &camera_manager) ||
      camera_manager < 0x80000000u ||
      !TryReadU32(guard, STATE_MANAGER + OBJECT_LIST_OFFSET, &object_list) ||
      object_list < 0x80000000u)
  {
    return 0.0f;
  }

  u32 camera_uid_word = 0;
  if (!TryReadU32(guard, camera_manager + CAMERA_UID_OFFSET, &camera_uid_word))
    return 0.0f;
  const u16 camera_uid = static_cast<u16>(camera_uid_word >> 16);
  if (camera_uid == 0xffffu)
    return 0.0f;

  u32 camera = 0;
  float yaw_deg = 0.0f;
  if (!TryReadU32(guard, object_list + ((camera_uid & 0x03ffu) << 3) + 4u, &camera) ||
      camera < 0x80000000u ||
      !ReadYawFromTransform2D(guard, camera + TRANSFORM_OFFSET, &yaw_deg))
  {
    return 0.0f;
  }

  return WrapRadians((yaw_deg + 180.0f) * (static_cast<float>(MathUtil::PI) / 180.0f)) *
         (-180.0f / static_cast<float>(MathUtil::PI));
}

#ifdef ENABLE_VR
void UpdateMovement(const Core::CPUThreadGuard& guard,
                    const Common::VR::OpenXRInputSnapshot& snapshot,
                    const RuntimeSettings& settings, u32 player, float yaw_delta_deg)
{
  if (!settings.directional_movement_enabled || !snapshot.runtime_active ||
      !snapshot.head_pose.valid || s_vr_menu_visible ||
      s_frame_counter < s_vr_menu_input_suppress_until_frame ||
      !PlayerIsFirstPerson(guard, player))
  {
    s_move_speed = 0.0f;
    return;
  }

  u32 orbit_state = 0;
  if (TryReadU32(guard, player + ORBIT_STATE_OFFSET, &orbit_state) && orbit_state != 0 &&
      orbit_state != ORBIT_STATE_GRAPPLE)
  {
    // Locked on: leave the game's own strafing/orbit movement alone.
    s_move_speed = 0.0f;
    return;
  }

  const int move_hand = settings.directional_movement_use_right_stick ? 1 : 0;
  const Common::VR::OpenXRControllerState& controller = snapshot.controllers[move_hand];
  if (!controller.connected ||
      (!settings.directional_movement_use_hmd_direction && !controller.aim_pose.valid))
  {
    s_move_speed = 0.0f;
    return;
  }

  // While the movement hand is at the head it is selecting a visor, not steering.
  if (controller.aim_pose.valid &&
      LeftControllerNearHead(PoseFromOpenXR(controller.aim_pose),
                             PoseFromOpenXR(snapshot.head_pose), settings))
  {
    s_move_speed = 0.0f;
    return;
  }

  const float stick_x = controller.thumbstick_x;
  const float stick_y = controller.thumbstick_y;
  const float stick_mag = std::sqrt(stick_x * stick_x + stick_y * stick_y);
  const float move_mag = std::min(stick_mag, 1.0f);
  if (move_mag < settings.directional_movement_deadzone)
  {
    s_move_speed = 0.0f;
    return;
  }

  float vx = 0.0f;
  float vy = 0.0f;
  float vz = 0.0f;
  if (!TryReadFloat(guard, player + VELOCITY_OFFSET + 0x00u, &vx) ||
      !TryReadFloat(guard, player + VELOCITY_OFFSET + 0x04u, &vy) ||
      !TryReadFloat(guard, player + VELOCITY_OFFSET + 0x08u, &vz) || !std::isfinite(vx) ||
      !std::isfinite(vy) || !std::isfinite(vz))
  {
    return;
  }

  const Pose hmd_pose = PoseFromOpenXR(snapshot.head_pose);
  const Pose move_pose = settings.directional_movement_use_hmd_direction ?
                             hmd_pose :
                             PoseFromOpenXR(controller.aim_pose);
  float base_yaw = 0.0f;
  if (!PosePrimeYaw(move_pose, yaw_delta_deg, &base_yaw))
  {
    s_move_speed = 0.0f;
    return;
  }

  float forward_x = 0.0f;
  float forward_y = 0.0f;
  float right_x = 0.0f;
  float right_y = 0.0f;
  PrimeXYFromYaw(WrapRadians(base_yaw + static_cast<float>(MathUtil::PI)), &forward_x,
                 &forward_y);
  PrimeXYFromYaw(WrapRadians(base_yaw + static_cast<float>(MathUtil::PI) * 0.5f), &right_x,
                 &right_y);

  float dir_x = forward_x * stick_y - right_x * stick_x;
  float dir_y = forward_y * stick_y - right_y * stick_x;
  const float dir_len = std::sqrt(dir_x * dir_x + dir_y * dir_y);
  if (!std::isfinite(dir_len) || dir_len < 0.0001f)
  {
    s_move_speed = 0.0f;
    return;
  }
  dir_x /= dir_len;
  dir_y /= dir_len;

  u32 move_state = 0xffffffffu;
  const bool on_ground = TryReadU32(guard, player + MOVE_STATE_OFFSET, &move_state) &&
                         move_state == 0;
  const float flat_speed = std::sqrt(vx * vx + vy * vy);
  const float target_speed = settings.directional_movement_speed * move_mag;
  if (s_move_speed <= 0.0f || s_move_speed > target_speed)
    s_move_speed = std::min(std::max(flat_speed, 0.0f), target_speed);

  const float accel = on_ground ? settings.directional_movement_accel :
                                  settings.directional_movement_air_accel;
  constexpr float frame_dt = 1.0f / 60.0f;
  const float max_delta = accel * frame_dt;
  if (s_move_speed < target_speed)
    s_move_speed = std::min(target_speed, s_move_speed + max_delta);
  else
    s_move_speed = std::max(target_speed, s_move_speed - max_delta);

  TryWriteFloat(guard, player + VELOCITY_OFFSET + 0x00u, dir_x * s_move_speed);
  TryWriteFloat(guard, player + VELOCITY_OFFSET + 0x04u, dir_y * s_move_speed);
  TryWriteFloat(guard, player + VELOCITY_OFFSET + 0x08u, vz);
}

void UpdateSnapTurn(const Core::CPUThreadGuard& guard,
                    const Common::VR::OpenXRInputSnapshot& snapshot,
                    const RuntimeSettings& settings, u32 player)
{
  if (!settings.snap_turn_enabled || !snapshot.runtime_active || s_vr_menu_visible ||
      s_frame_counter < s_vr_menu_input_suppress_until_frame)
  {
    if (!settings.snap_turn_enabled)
      s_snap_ready = true;
    return;
  }

  constexpr size_t right_hand_index = 1;
  const Common::VR::OpenXRControllerState& controller = snapshot.controllers[right_hand_index];
  if (!controller.connected)
  {
    s_snap_ready = true;
    s_snap_cooldown_until_frame = 0;
    return;
  }

  // While this hand is at the head it is selecting a beam, not snap-turning.
  if (snapshot.head_pose.valid && controller.aim_pose.valid &&
      LeftControllerNearHead(PoseFromOpenXR(controller.aim_pose),
                             PoseFromOpenXR(snapshot.head_pose), settings))
  {
    s_snap_ready = false;
    return;
  }

  constexpr float snap_enter_threshold = 0.72f;
  constexpr float snap_exit_threshold = 0.32f;
  const float stick_x = std::clamp(controller.thumbstick_x, -1.0f, 1.0f);
  if (std::fabs(stick_x) <= snap_exit_threshold)
  {
    s_snap_ready = true;
    return;
  }

  if (!s_snap_ready || std::fabs(stick_x) < snap_enter_threshold ||
      s_frame_counter < s_snap_cooldown_until_frame)
  {
    return;
  }

  // Consume the flick even when gameplay is unavailable so holding the stick through a
  // transition cannot snap on the next valid frame (same policy as Prime 1).
  s_snap_ready = false;
  if (!PlayerIsFirstPerson(guard, player))
    return;

  const int degrees = std::clamp(settings.snap_turn_degrees, SNAP_TURN_DEGREES_CHOICES.front(),
                                 SNAP_TURN_DEGREES_CHOICES.back());
  const float signed_degrees = (stick_x > 0.0f ? 1.0f : -1.0f) * static_cast<float>(degrees);
  const float yaw_delta_rad = signed_degrees * (static_cast<float>(MathUtil::PI) / 180.0f);
  if (RotateTransformYaw2D(guard, player + TRANSFORM_OFFSET, yaw_delta_rad))
    s_snap_cooldown_until_frame = s_frame_counter + 8;
}
#endif

// RE probe (slice 2c): locate the CPlayerGun* inside CPlayer. A candidate is a heap pointer
// whose vtable has entries inside CPlayerGun.cpp's .text range (exact bounds from the
// PrimeDecomp/echoes G2ME01 splits). Logged only under QPVR_PG_LOG; removed once the offset
// is confirmed and hardcoded.
constexpr u32 GUN_TEXT_BEGIN = 0x801C6DF8u;
constexpr u32 GUN_TEXT_END = 0x801D2F0Cu;
constexpr u32 VTABLE_REGION_BEGIN = 0x803A56C0u;  // .rodata begin
constexpr u32 VTABLE_REGION_END = 0x803C5A10u;    // .data end

// Confirmed 2026-07-13: single hit at player+0xEBC (vtable 0x803B716C, 10/16 entries in
// CPlayerGun.cpp text, stable across a world reload). Kept for regression checking.
void LogGunPointerScan(const Core::CPUThreadGuard& guard, u32 player)
{
  u32 gun = 0;
  u32 vtable = 0;
  if (TryReadU32(guard, player + GUN_OFFSET, &gun) && gun >= 0x80000000u &&
      TryReadU32(guard, gun, &vtable))
  {
    NOTICE_LOG_FMT(CORE, "PrimedGun prime2 gun_scan player+{:04X} obj={:08X} vtable={:08X}",
                   GUN_OFFSET, gun, vtable);
  }
}

// RE probe (slice 2c, layer 2): map CTransform-shaped 3x4 float blocks inside CPlayerGun.
// A candidate has three near-orthonormal basis rows and a sane translation. Logged with
// the active camera transform alongside, so offline analysis can classify which block is
// the aim/world transform (translation near camera, forward tracking view) vs local/model.
void LogGunTransformScan(const Core::CPUThreadGuard& guard, u32 player)
{
  u32 gun = 0;
  if (!TryReadU32(guard, player + GUN_OFFSET, &gun) || gun < 0x80000000u)
    return;

  float cam_fx = 0.0f;
  float cam_fy = 0.0f;
  float cam_tx = 0.0f;
  float cam_ty = 0.0f;
  float cam_tz = 0.0f;
  u32 camera_manager = 0;
  u32 object_list = 0;
  u32 camera_uid_word = 0;
  u32 camera = 0;
  if (TryReadU32(guard, STATE_MANAGER + CAMERA_MANAGER_OFFSET, &camera_manager) &&
      camera_manager >= 0x80000000u &&
      TryReadU32(guard, STATE_MANAGER + OBJECT_LIST_OFFSET, &object_list) &&
      object_list >= 0x80000000u &&
      TryReadU32(guard, camera_manager + CAMERA_UID_OFFSET, &camera_uid_word) &&
      static_cast<u16>(camera_uid_word >> 16) != 0xffffu &&
      TryReadU32(guard,
                 object_list + (((camera_uid_word >> 16) & 0x03ffu) << 3) + 4u, &camera) &&
      camera >= 0x80000000u)
  {
    const u32 cam_xf = camera + TRANSFORM_OFFSET;
    TryReadFloat(guard, cam_xf + 0x04u, &cam_fx);
    TryReadFloat(guard, cam_xf + 0x14u, &cam_fy);
    TryReadFloat(guard, cam_xf + 0x0Cu, &cam_tx);
    TryReadFloat(guard, cam_xf + 0x1Cu, &cam_ty);
    TryReadFloat(guard, cam_xf + 0x2Cu, &cam_tz);
  }
  NOTICE_LOG_FMT(CORE, "PrimedGun prime2 gunxf_cam fwd=({:.3f},{:.3f}) pos=({:.2f},{:.2f},{:.2f})",
                 cam_fx, cam_fy, cam_tx, cam_ty, cam_tz);

  u32 logged = 0;
  for (u32 offset = 0u; offset + 0x30u <= 0x800u && logged < 10u; offset += 4u)
  {
    std::array<float, 12> m{};
    bool ok = true;
    for (u32 i = 0; i < m.size(); ++i)
    {
      if (!TryReadFloat(guard, gun + offset + i * 4u, &m[i]) || !std::isfinite(m[i]))
      {
        ok = false;
        break;
      }
    }
    if (!ok)
      continue;

    const auto row_len = [&](u32 r) {
      return std::sqrt(m[r * 4 + 0] * m[r * 4 + 0] + m[r * 4 + 1] * m[r * 4 + 1] +
                       m[r * 4 + 2] * m[r * 4 + 2]);
    };
    const auto row_dot = [&](u32 a, u32 b) {
      return std::fabs(m[a * 4 + 0] * m[b * 4 + 0] + m[a * 4 + 1] * m[b * 4 + 1] +
                       m[a * 4 + 2] * m[b * 4 + 2]);
    };
    const float l0 = row_len(0);
    const float l1 = row_len(1);
    const float l2 = row_len(2);
    if (l0 < 0.9f || l0 > 1.1f || l1 < 0.9f || l1 > 1.1f || l2 < 0.9f || l2 > 1.1f ||
        row_dot(0, 1) > 0.1f || row_dot(0, 2) > 0.1f || row_dot(1, 2) > 0.1f)
    {
      continue;
    }
    const float tx = m[3];
    const float ty = m[7];
    const float tz = m[11];
    if (std::fabs(tx) > 10000.0f || std::fabs(ty) > 10000.0f || std::fabs(tz) > 10000.0f)
      continue;

    ++logged;
    NOTICE_LOG_FMT(CORE,
                   "PrimedGun prime2 gunxf gun+{:04X} fwd=({:.3f},{:.3f},{:.3f}) "
                   "pos=({:.2f},{:.2f},{:.2f})",
                   offset, m[1], m[5], m[9], tx, ty, tz);
  }
}

constexpr u32 GUN_WORLD_XF_OFFSET = 0x24u;      // gun world transform (tracks camera aim)
constexpr u32 GUN_WORLD_XF_TWIN_OFFSET = 0x54u; // identical twin copy (grip/beam analog)

// Mid-update hook: host frame-end writes to gun+0x24 land but the game recomputes the
// transform during its own update before render (verified — probe reads back host values,
// headset shows no change). A write watchpoint traced the real writer to the paired-single
// copy inside CTransform4f::operator= (0x802C9020, named in the decomp; SetTransformAlt was
// a dead end — its call counter stayed 0). The cave sits on operator= entry: when the dest
// (r3) is the gun's world transform (or its +0x54 twin), it bumps a counter and, in redirect
// mode, swaps the source (r4) to a host-written matrix — so the game copies the controller
// pose into itself with its own code, at the right point in the frame. Cave+scratch live in
// PrimeHack's vetted free-RAM zone for G2ME01 (0x80420000), NOT in the Prime 1 arenas.
// Scratch: +0 dest A (gun+0x24), +4 dest B (gun+0x54), +8 mode (1=redirect), +0xC call
// counter, +0x10 CTransform4f (12 floats).
constexpr u32 P2_CAVE_BASE = 0x80420000u;
constexpr u32 P2_SCRATCH_BASE = 0x80420100u;
constexpr u32 XF_ASSIGN_ADDRESS = 0x802C9020u;  // __as__12CTransform4fFRC12CTransform4f
constexpr u32 XF_ASSIGN_RETURN = 0x802C9024u;
constexpr u32 RETIRED_SET_TRANSFORM_ALT_ADDRESS = 0x80049F58u;
constexpr u32 RETIRED_SET_TRANSFORM_ALT_ORIGINAL = 0x9421FFF0u;  // stwu r1,-0x10(r1)

// PAD-level input injection: one cave wrapping PADRead (SDK, named symbol) replaces Prime
// 1's d-pad/suppression cave set. Entry saves the caller's LR + the PADStatus pointer and
// reroutes the return into a post-handler, which ORs a host-written button mask into pad 0
// and optionally overrides the C-stick bytes — after the SDK filled real data, before the
// game reads it, so the engine's own edge detection treats injections as native presses.
// Scratch: +0 saved LR, +4 saved PADStatus*, +8 button OR-mask (PAD_BUTTON_* u16),
// +0xC C-stick mode (1 = override), +0x10 packed substick X<<8|Y (s8 bytes).
constexpr u32 P2_PAD_CAVE = 0x80420400u;
constexpr u32 P2_PAD_SCRATCH = 0x80420200u;
constexpr u32 PAD_READ_ADDRESS = 0x8037748Cu;  // PADRead
constexpr u32 PAD_READ_RETURN = 0x80377490u;
constexpr u16 PAD_DPAD_LEFT = 0x0001u;
constexpr u16 PAD_DPAD_RIGHT = 0x0002u;
constexpr u16 PAD_DPAD_DOWN = 0x0004u;
constexpr u16 PAD_DPAD_UP = 0x0008u;

#ifdef ENABLE_VR
// Slice-2c experiment: drive the gun's world transform from the controller pose each frame,
// anchored at the camera position. The game recomputes these matrices during its own update,
// so frame-end writes may fight the game's writer — this establishes whether gun+0x24/+0x54
// feed the render before a mid-update hook is built. Gated by QPVR_P2_HANDGUN=1.
bool HandGunExperimentEnabled()
{
  static const bool enabled = []() {
    const char* value = std::getenv("QPVR_P2_HANDGUN");
    return value != nullptr && value[0] == '1';
  }();
  return enabled;
}

bool ResolveCameraTransformAddr(const Core::CPUThreadGuard& guard, u32* out)
{
  u32 camera_manager = 0;
  u32 object_list = 0;
  u32 camera_uid_word = 0;
  u32 camera = 0;
  if (!TryReadU32(guard, STATE_MANAGER + CAMERA_MANAGER_OFFSET, &camera_manager) ||
      camera_manager < 0x80000000u ||
      !TryReadU32(guard, STATE_MANAGER + OBJECT_LIST_OFFSET, &object_list) ||
      object_list < 0x80000000u ||
      !TryReadU32(guard, camera_manager + CAMERA_UID_OFFSET, &camera_uid_word) ||
      static_cast<u16>(camera_uid_word >> 16) == 0xffffu ||
      !TryReadU32(guard, object_list + (((camera_uid_word >> 16) & 0x03ffu) << 3) + 4u,
                  &camera) ||
      camera < 0x80000000u)
  {
    return false;
  }
  *out = camera + TRANSFORM_OFFSET;
  return true;
}

void WriteGunTransform(const Core::CPUThreadGuard& guard, u32 xf, const Matrix3x4& basis,
                       float tx, float ty, float tz)
{
  for (u32 row = 0; row < 3u; ++row)
  {
    for (u32 col = 0; col < 3u; ++col)
    {
      TryWriteFloat(guard, xf + row * 0x10u + col * 4u,
                    basis.At(static_cast<int>(row), static_cast<int>(col)));
    }
  }
  TryWriteFloat(guard, xf + 0x0Cu, tx);
  TryWriteFloat(guard, xf + 0x1Cu, ty);
  TryWriteFloat(guard, xf + 0x2Cu, tz);
}

// Installs (and heals) the CTransform4f::operator= cave. Returns true when instructions
// were written this call, so the caller can invalidate the JIT ICache.
bool InstallGunTransformHook(const Core::CPUThreadGuard& guard)
{
  // A savestate from the retired SetTransformAlt experiment could resurrect its branch;
  // put the known prologue back if so.
  u32 retired = 0;
  if (TryReadU32(guard, RETIRED_SET_TRANSFORM_ALT_ADDRESS, &retired) &&
      retired == PpcBranch(RETIRED_SET_TRANSFORM_ALT_ADDRESS, P2_CAVE_BASE))
  {
    TryWriteInstruction(guard, RETIRED_SET_TRANSFORM_ALT_ADDRESS,
                        RETIRED_SET_TRANSFORM_ALT_ORIGINAL);
  }

  const u32 branch_to_cave = PpcBranch(XF_ASSIGN_ADDRESS, P2_CAVE_BASE);
  u32 site = 0;
  if (!TryReadU32(guard, XF_ASSIGN_ADDRESS, &site))
    return false;

  if (site != branch_to_cave)
  {
    // Capture the displaced instruction; refuse PC-relative originals (branch classes)
    // since they cannot be relocated into the cave.
    const u32 opcode = site >> 26;
    if (site == 0u || opcode == 18u || opcode == 16u)
    {
      NOTICE_LOG_FMT(CORE, "PrimedGun prime2 gunhook ABORT: unsafe original {:08X}", site);
      return false;
    }
    s_gun_hook_original = site;
  }
  else if (s_gun_hook_original == 0u)
  {
    // Branch already present (e.g. savestate) but the original was lost; leave it be.
    return false;
  }

  const u32 branch_back = PpcBranch(P2_CAVE_BASE + 0x40u, XF_ASSIGN_RETURN);
  const std::initializer_list<HookWrite> cave = {
      {0x00u, 0x3D808042u},          // lis   r12, 0x8042
      {0x04u, 0x618C0100u},          // ori   r12, r12, 0x0100  (scratch base)
      {0x08u, 0x816C0000u},          // lwz   r11, 0(r12)       (dest A: gun world xf)
      {0x0Cu, 0x7C035800u},          // cmpw  r3, r11
      {0x10u, 0x41820010u},          // beq   +0x10 -> matched
      {0x14u, 0x816C0004u},          // lwz   r11, 4(r12)       (dest B: twin)
      {0x18u, 0x7C035800u},          // cmpw  r3, r11
      {0x1Cu, 0x40820020u},          // bne   +0x20 -> original
      {0x20u, 0x816C000Cu},          // matched: lwz r11, 0xC(r12)  (counter)
      {0x24u, 0x396B0001u},          // addi  r11, r11, 1
      {0x28u, 0x916C000Cu},          // stw   r11, 0xC(r12)
      {0x2Cu, 0x816C0008u},          // lwz   r11, 8(r12)       (mode)
      {0x30u, 0x2C0B0001u},          // cmpwi r11, 1
      {0x34u, 0x40820008u},          // bne   +0x8 -> original
      {0x38u, 0x388C0010u},          // addi  r4, r12, 0x10     (redirect source matrix)
      {0x3Cu, s_gun_hook_original},  // displaced original instruction
      {0x40u, branch_back},          // b     operator=+4
  };

  bool wrote = false;
  if (!InstructionBlockMatches(guard, P2_CAVE_BASE, cave))
  {
    if (!s_gun_hook_installed)
    {
      // First install: zero the scratch block so the cave starts inert.
      for (u32 offset = 0u; offset < 0x40u; offset += 4u)
        TryWriteU32(guard, P2_SCRATCH_BASE + offset, 0u);
    }
    wrote = TryWriteInstructionBlock(guard, P2_CAVE_BASE, cave) || wrote;
  }
  if (site != branch_to_cave)
    wrote = TryWriteInstruction(guard, XF_ASSIGN_ADDRESS, branch_to_cave) || wrote;

  if (wrote && !s_gun_hook_installed)
  {
    s_gun_hook_installed = true;
    NOTICE_LOG_FMT(CORE, "PrimedGun prime2 gunhook installed at {:08X} (orig {:08X})",
                   XF_ASSIGN_ADDRESS, s_gun_hook_original);
  }
  return wrote;
}

void InvalidateGunHookICache(Core::System& system)
{
  auto& jit = system.GetJitInterface();
  jit.InvalidateICache(XF_ASSIGN_ADDRESS & ~0xFFFu, 0x1000u, true);
  jit.InvalidateICache(RETIRED_SET_TRANSFORM_ALT_ADDRESS & ~0xFFFu, 0x1000u, true);
  jit.InvalidateICache(P2_CAVE_BASE, 0x100u, true);
}

struct GestureState
{
  bool ready = true;
  u64 hold_until_frame = 0;
  u32 held_value = 0;
};
GestureState s_visor_gesture;
GestureState s_beam_gesture;

bool InstallPadHook(const Core::CPUThreadGuard& guard)
{
  const u32 branch_to_cave = PpcBranch(PAD_READ_ADDRESS, P2_PAD_CAVE);
  u32 site = 0;
  if (!TryReadU32(guard, PAD_READ_ADDRESS, &site))
    return false;

  if (site != branch_to_cave)
  {
    const u32 opcode = site >> 26;
    if (site == 0u || opcode == 18u || opcode == 16u)
    {
      NOTICE_LOG_FMT(CORE, "PrimedGun prime2 padhook ABORT: unsafe original {:08X}", site);
      return false;
    }
    s_pad_hook_original = site;
  }
  else if (s_pad_hook_original == 0u)
  {
    return false;
  }

  const u32 post = P2_PAD_CAVE + 0x28u;
  const std::initializer_list<HookWrite> cave = {
      {0x00u, 0x3D808042u},                          // lis   r12, 0x8042
      {0x04u, 0x618C0000u | (P2_PAD_SCRATCH & 0xFFFFu)},  // ori r12, r12, scratch_lo
      {0x08u, 0x7D6802A6u},                          // mflr  r11
      {0x0Cu, 0x916C0000u},                          // stw   r11, 0(r12)   (caller LR)
      {0x10u, 0x906C0004u},                          // stw   r3, 4(r12)    (PADStatus*)
      {0x14u, 0x3D608042u},                          // lis   r11, 0x8042
      {0x18u, 0x616B0000u | (post & 0xFFFFu)},       // ori   r11, r11, post_lo
      {0x1Cu, 0x7D6803A6u},                          // mtlr  r11           (return -> post)
      {0x20u, s_pad_hook_original},                  // displaced original instruction
      {0x24u, PpcBranch(P2_PAD_CAVE + 0x24u, PAD_READ_RETURN)},  // b PADRead+4
      // post-handler: PADRead's blr lands here with r3 = return value (preserved).
      // mode 0 = passthrough, 1 = override C-stick, 2 = menu suppress (zero buttons+sticks).
      {0x28u, 0x3D808042u},                          // lis   r12, 0x8042
      {0x2Cu, 0x618C0000u | (P2_PAD_SCRATCH & 0xFFFFu)},  // ori r12, r12, scratch_lo
      {0x30u, 0x816C0004u},                          // lwz   r11, 4(r12)   (PADStatus*)
      {0x34u, 0x808C000Cu},                          // lwz   r4, 0xC(r12)  (mode)
      {0x38u, 0x2C040002u},                          // cmpwi r4, 2
      {0x3Cu, 0x4182002Cu},                          // beq   +0x2C -> menu suppress
      {0x40u, 0xA00B0000u},                          // lhz   r0, 0(r11)    (pad0 buttons)
      {0x44u, 0x808C0008u},                          // lwz   r4, 8(r12)    (OR mask)
      {0x48u, 0x7C002378u},                          // or    r0, r0, r4
      {0x4Cu, 0xB00B0000u},                          // sth   r0, 0(r11)
      {0x50u, 0x808C000Cu},                          // lwz   r4, 0xC(r12)  (mode again)
      {0x54u, 0x2C040001u},                          // cmpwi r4, 1
      {0x58u, 0x40820020u},                          // bne   +0x20 -> done
      {0x5Cu, 0x808C0010u},                          // lwz   r4, 0x10(r12) (packed substick)
      {0x60u, 0xB08B0004u},                          // sth   r4, 4(r11)    (substick X/Y)
      {0x64u, 0x48000014u},                          // b     +0x14 -> done
      {0x68u, 0x38000000u},                          // menu: li r0, 0
      {0x6Cu, 0xB00B0000u},                          // sth   r0, 0(r11)    (buttons)
      {0x70u, 0xB00B0002u},                          // sth   r0, 2(r11)    (main stick X/Y)
      {0x74u, 0xB00B0004u},                          // sth   r0, 4(r11)    (substick X/Y)
      {0x78u, 0x816C0000u},                          // done: lwz r11, 0(r12)
      {0x7Cu, 0x7D6803A6u},                          // mtlr  r11
      {0x80u, 0x4E800020u},                          // blr
  };

  bool wrote = false;
  if (!InstructionBlockMatches(guard, P2_PAD_CAVE, cave))
  {
    if (!s_pad_hook_installed)
    {
      for (u32 offset = 0u; offset < 0x18u; offset += 4u)
        TryWriteU32(guard, P2_PAD_SCRATCH + offset, 0u);
    }
    wrote = TryWriteInstructionBlock(guard, P2_PAD_CAVE, cave) || wrote;
  }
  if (site != branch_to_cave)
    wrote = TryWriteInstruction(guard, PAD_READ_ADDRESS, branch_to_cave) || wrote;

  if (wrote && !s_pad_hook_installed)
  {
    s_pad_hook_installed = true;
    NOTICE_LOG_FMT(CORE, "PrimedGun prime2 padhook installed at {:08X} (orig {:08X})",
                   PAD_READ_ADDRESS, s_pad_hook_original);
  }
  return wrote;
}

void InvalidatePadHookICache(Core::System& system)
{
  auto& jit = system.GetJitInterface();
  jit.InvalidateICache(PAD_READ_ADDRESS & ~0xFFFu, 0x1000u, true);
  jit.InvalidateICache(P2_PAD_CAVE, 0x100u, true);
}

u32 DpadDirToPadMask(DpadDir dir)
{
  switch (dir)
  {
  case DpadUp:
    return PAD_DPAD_UP;
  case DpadDown:
    return PAD_DPAD_DOWN;
  case DpadLeft:
    return PAD_DPAD_LEFT;
  case DpadRight:
    return PAD_DPAD_RIGHT;
  default:
    return 0;
  }
}

// Packed substick X<<8|Y as signed bytes; +/-70 survives PADClamp's octagonal gate.
u32 DpadDirToSubstick(DpadDir dir)
{
  switch (dir)
  {
  case DpadUp:
    return 0x0046u;
  case DpadDown:
    return 0x00BAu;
  case DpadLeft:
    return 0xBA00u;
  case DpadRight:
    return 0x4600u;
  default:
    return 0;
  }
}

DpadDir GestureFlick(GestureState* state, bool near_head, float stick_x, float stick_y)
{
  if (!near_head)
  {
    state->ready = true;
    return DpadNone;
  }
  const float mag = std::sqrt(stick_x * stick_x + stick_y * stick_y);
  if (mag < 0.30f)
  {
    state->ready = true;
    return DpadNone;
  }
  if (!state->ready || mag < 0.70f)
    return DpadNone;
  state->ready = false;
  return StickToDpad(stick_x, stick_y, 0.5f, DpadNone);
}

// Off-hand near head + stick flick -> visor d-pad press; cannon hand near head + flick ->
// beam C-stick press. While snap turn owns the physical right stick, the game's C-stick is
// held centered so flicks do not also switch beams (beams go through the gesture instead).
void UpdateXrGestures(const Core::CPUThreadGuard& guard,
                      const Common::VR::OpenXRInputSnapshot& snapshot,
                      const RuntimeSettings& settings, u32 player)
{
  u32 button_mask = 0;
  u32 cstick_mode = 0;
  u32 cstick_value = 0;

  if (s_vr_menu_visible)
  {
    // The user is in the PrimedGun menu; the game must not see any pad input.
    TryWriteU32(guard, P2_PAD_SCRATCH + 0x8u, 0u);
    TryWriteU32(guard, P2_PAD_SCRATCH + 0xCu, 2u);
    TryWriteU32(guard, P2_PAD_SCRATCH + 0x10u, 0u);
    return;
  }

  if (settings.patch_xr_visor_dpad_timing && snapshot.runtime_active &&
      snapshot.head_pose.valid && PlayerIsFirstPerson(guard, player))
  {
    const Pose hmd = PoseFromOpenXR(snapshot.head_pose);
    const int main_hand = settings.use_right_hand ? 1 : 0;
    const int off_hand = 1 - main_hand;

    const Common::VR::OpenXRControllerState& off = snapshot.controllers[off_hand];
    const bool off_near = off.connected && off.aim_pose.valid &&
                          LeftControllerNearHead(PoseFromOpenXR(off.aim_pose), hmd, settings);
    const DpadDir visor = GestureFlick(&s_visor_gesture, off_near, off.thumbstick_x,
                                       off.thumbstick_y);
    if (visor != DpadNone)
    {
      s_visor_gesture.held_value = DpadDirToPadMask(visor);
      s_visor_gesture.hold_until_frame = s_frame_counter + 6;
    }
    if (s_frame_counter < s_visor_gesture.hold_until_frame)
      button_mask |= s_visor_gesture.held_value;

    const Common::VR::OpenXRControllerState& main = snapshot.controllers[main_hand];
    const bool main_near = main.connected && main.aim_pose.valid &&
                           LeftControllerNearHead(PoseFromOpenXR(main.aim_pose), hmd, settings);
    const DpadDir beam = GestureFlick(&s_beam_gesture, main_near, main.thumbstick_x,
                                      main.thumbstick_y);
    if (beam != DpadNone)
    {
      s_beam_gesture.held_value = DpadDirToSubstick(beam);
      s_beam_gesture.hold_until_frame = s_frame_counter + 6;
    }
    if (s_frame_counter < s_beam_gesture.hold_until_frame)
    {
      cstick_mode = 1;
      cstick_value = s_beam_gesture.held_value;
    }
    else if (settings.snap_turn_enabled)
    {
      cstick_mode = 1;
      cstick_value = 0;
    }
  }

  TryWriteU32(guard, P2_PAD_SCRATCH + 0x8u, button_mask);
  TryWriteU32(guard, P2_PAD_SCRATCH + 0xCu, cstick_mode);
  TryWriteU32(guard, P2_PAD_SCRATCH + 0x10u, cstick_value);
}

// RE probe (slice 2c, plan C): the SetTransformAlt hook proved the gun transform is written
// by inline code (call counter stayed 0), so a write watchpoint on gun+0x24 has the emulator
// log the writer's PC ("MBP ..." lines, MEMMAP category). Gated by QPVR_P2_FINDSTORE=1;
// re-armed automatically when the gun object is recreated on world loads.
bool FindStoreEnabled()
{
  static const bool enabled = []() {
    const char* value = std::getenv("QPVR_P2_FINDSTORE");
    return value != nullptr && value[0] == '1';
  }();
  return enabled;
}

u32 s_watched_gun = 0;

void UpdateGunWriteWatch(Core::System& system, u32 gun)
{
  if (!FindStoreEnabled() || gun == s_watched_gun)
    return;

  auto& memchecks = system.GetPowerPC().GetMemChecks();
  if (s_watched_gun != 0)
    memchecks.Remove(s_watched_gun + GUN_WORLD_XF_OFFSET);
  if (gun != 0)
  {
    Common::Log::LogManager::GetInstance()->SetEnable(Common::Log::LogType::MEMMAP, true);
    TMemCheck check;
    check.start_address = gun + GUN_WORLD_XF_OFFSET;
    check.end_address = gun + GUN_WORLD_XF_OFFSET + 3u;
    check.is_ranged = true;
    check.is_break_on_write = true;
    check.log_on_hit = true;
    check.break_on_hit = false;
    memchecks.Add(std::move(check));
    NOTICE_LOG_FMT(CORE, "PrimedGun prime2 write-watch armed at {:08X}",
                   gun + GUN_WORLD_XF_OFFSET);
  }
  s_watched_gun = gun;
}

void UpdateHandGunExperiment(Core::System& system, const Core::CPUThreadGuard& guard,
                             const Common::VR::OpenXRInputSnapshot& snapshot,
                             const RuntimeSettings& settings, u32 player, float yaw_delta_deg)
{
  if (!HandGunExperimentEnabled())
    return;

  if (InstallGunTransformHook(guard))
    InvalidateGunHookICache(system);

  u32 gun = 0;
  const bool gun_ok = TryReadU32(guard, player + GUN_OFFSET, &gun) && gun >= 0x80000000u;
  TryWriteU32(guard, P2_SCRATCH_BASE + 0x0u, gun_ok ? gun + GUN_WORLD_XF_OFFSET : 0u);
  TryWriteU32(guard, P2_SCRATCH_BASE + 0x4u, gun_ok ? gun + GUN_WORLD_XF_TWIN_OFFSET : 0u);
  UpdateGunWriteWatch(system, gun_ok ? gun : 0u);

  bool redirect = false;
  const Common::VR::OpenXRControllerState& hand =
      snapshot.controllers[settings.use_right_hand ? 1 : 0];
  u32 cam_xf = 0;
  float cx = 0.0f;
  float cy = 0.0f;
  float cz = 0.0f;
  if (gun_ok && snapshot.head_pose.valid && hand.connected && hand.aim_pose.valid &&
      PlayerIsFirstPerson(guard, player) && ResolveCameraTransformAddr(guard, &cam_xf) &&
      TryReadFloat(guard, cam_xf + 0x0Cu, &cx) && TryReadFloat(guard, cam_xf + 0x1Cu, &cy) &&
      TryReadFloat(guard, cam_xf + 0x2Cu, &cz))
  {
    Pose pose = PoseFromOpenXR(hand.aim_pose);
    ApplyTrackingWorldYaw(pose, yaw_delta_deg);
    const Matrix3x4 basis =
        PoseToPrimeMatrix(pose, 0.0f, 0.0f, 0.0f, settings.rot_offset_x, settings.rot_offset_y,
                          settings.rot_offset_z, settings.world_scale);

    // Controller position relative to the head, rotated into game-world yaw, mapped to
    // Prime axes ((x, -z, y) * scale), anchored at the camera.
    const float rel_x = hand.aim_pose.position[0] - snapshot.head_pose.position[0];
    const float rel_y = hand.aim_pose.position[1] - snapshot.head_pose.position[1];
    const float rel_z = hand.aim_pose.position[2] - snapshot.head_pose.position[2];
    const float yaw_rad = yaw_delta_deg * (static_cast<float>(MathUtil::PI) / 180.0f);
    const float cos_yaw = std::cos(yaw_rad);
    const float sin_yaw = std::sin(yaw_rad);
    const float rot_x = rel_x * cos_yaw + rel_z * sin_yaw;
    const float rot_z = -rel_x * sin_yaw + rel_z * cos_yaw;
    // Prime's basis convention carries a 180-degree yaw flip (see the +PI in the movement
    // yaw math and the negations in PoseToPrimeMatrix); the horizontal offset needs the
    // same flip or left/right and forward/back mirror (user-confirmed in headset).
    const float gx = cx - rot_x * settings.world_scale;
    const float gy = cy + rot_z * settings.world_scale;
    const float gz = cz + rel_y * settings.world_scale;

    if (MatrixNumbersLookValid(basis) && std::isfinite(gx) && std::isfinite(gy) &&
        std::isfinite(gz))
    {
      WriteGunTransform(guard, P2_SCRATCH_BASE + 0x10u, basis, gx, gy, gz);
      redirect = true;
    }
  }
  TryWriteU32(guard, P2_SCRATCH_BASE + 0x8u, redirect ? 1u : 0u);

  if (RuntimeLoggingEnabled() && s_frame_counter >= s_gun_hook_log_frame + 300u)
  {
    s_gun_hook_log_frame = s_frame_counter;
    u32 counter = 0;
    TryReadU32(guard, P2_SCRATCH_BASE + 0xCu, &counter);
    NOTICE_LOG_FMT(CORE, "PrimedGun prime2 gunhook counter={} redirect={} gun={:08X}", counter,
                   redirect, gun);
  }
}
#endif

void OnFrameEnd(Core::System& system, const Core::CPUThreadGuard& guard,
                const RuntimeSettings& settings)
{
  s_was_active = true;

  if (GameBooted(guard) && ApplyPatches(guard, settings))
    InvalidatePatchedICache(system);

  if ((s_frame_counter % 60u) == 1u)
    ApplyHelmetOpacity(guard, settings);

  // Host-truth pause flag for the render-side 2D-lock. The game's menu_state is nonzero on
  // the pause/inventory/map and frontend screens but stays zero during cutscenes (verified:
  // the cannon-transform redirect kept running through landing cinematics). The classifier's
  // "primedgun_map_or_pause" flag is unusable for Prime 2 — its Map/MapOrHint verdicts fire
  // every frame of ordinary gameplay (trace census: 'Map' count ≈ per-frame 'Helmet' count),
  // which silently suppressed the cutscene/effect locks whenever that flag gated them.
  u32 p2_menu_state = 0;
  if (TryReadU32(guard, STATE_MANAGER + MENU_STATE_OFFSET, &p2_menu_state) &&
      p2_menu_state != 0u)
  {
    ShaderHunter::GetInstance().RegisterExternalFlag("prime2_pause");
  }
  // Telemetry: if the 2D-lock flip-flops, the log must say which input oscillates —
  // the render-side scene latch (logged in VertexManagerBase) or this pause signal.
  static u32 s_last_logged_menu_state = 0xffffffffu;
  if (RuntimeLoggingEnabled() && p2_menu_state != s_last_logged_menu_state)
  {
    NOTICE_LOG_FMT(CORE, "PrimedGun prime2 menu_state -> {} (frame {})", p2_menu_state,
                   s_frame_counter);
    s_last_logged_menu_state = p2_menu_state;
  }

  RuntimeSettings live_settings = settings;
#ifdef ENABLE_VR
  // The PrimedGun VR menu, overlay/layout publishing, and height recenter are pure host
  // features (no game-memory access), so Prime 2 runs them wholesale: left-stick press
  // opens the same menu, the published overlay state drives the same controller layout,
  // and cannon-hand stick press recenters height — all shared with Prime 1.
  const Common::VR::OpenXRInputSnapshot snapshot = Common::VR::OpenXRInputState::GetSnapshot();
  if (snapshot.runtime_active)
  {
    UpdateVrMenu(snapshot, &live_settings, false);
    SetRuntimeSettings(live_settings);
    UpdateHeightOnlyReset(snapshot,
                          snapshot.controllers[live_settings.use_right_hand ? 1 : 0],
                          live_settings);
  }
  else
  {
    PublishVrOverlayState(live_settings, false);
  }
#endif

  u32 player = 0;
  if (!ResolveGameplayPlayer(guard, &player))
  {
    s_move_speed = 0.0f;
    // Keep the pad injector inert through menus/loads so a mid-flick mask cannot stick.
    TryWriteU32(guard, P2_PAD_SCRATCH + 0x8u, 0u);
    TryWriteU32(guard, P2_PAD_SCRATCH + 0xCu, 0u);
    s_gameplay_input_active.store(false, std::memory_order_relaxed);
    s_orbit_lock_active.store(false, std::memory_order_relaxed);
    return;
  }

  // Host-truth scene signal for the render-side 2D-lock: gameplay resolution succeeding
  // means world loaded + live player + not in a menu — true through cutscenes as well
  // (the cannon redirect ran during landing cinematics), false on menus, load screens,
  // and the frontend. The render side unions this with its perspective-draw count, which
  // covers the moments a cutscene tears the player down (world-transition movies).
  ShaderHunter::GetInstance().RegisterExternalFlag("prime2_scene");

  // The HMD supplies pitch at the render layer; keep the game camera level. Writing every
  // frame (not only on change) also covers Echoes' one-frame pitch flicker after unmorphing.
  const bool first_person = PlayerIsFirstPerson(guard, player);
  if (first_person)
    TryWriteFloat(guard, player + PITCH_OFFSET, 0.0f);

  // Publish the same gameplay/orbit atomics the Prime 1 path feeds: GCPadEmu's modern
  // control layout (trigger = shoot, combat/lock-on stick modes) keys off these; without
  // them the pad stays in "classic" mode with GameCube-native mappings.
  u32 orbit_state = 0;
  TryReadU32(guard, player + ORBIT_STATE_OFFSET, &orbit_state);
  s_gameplay_input_active.store(first_person, std::memory_order_relaxed);
  s_orbit_lock_active.store(first_person && orbit_state != 0u &&
                                orbit_state != ORBIT_STATE_GRAPPLE,
                            std::memory_order_relaxed);

#ifdef ENABLE_VR
  if (snapshot.runtime_active)
  {
    const float yaw_delta_deg = GetYawDeltaDegrees(guard);
    UpdateMovement(guard, snapshot, live_settings, player, yaw_delta_deg);
    UpdateSnapTurn(guard, snapshot, live_settings, player);
    UpdateHandGunExperiment(system, guard, snapshot, live_settings, player, yaw_delta_deg);
    if (live_settings.patch_xr_visor_dpad_timing && InstallPadHook(guard))
      InvalidatePadHookICache(system);
    UpdateXrGestures(guard, snapshot, live_settings, player);
  }
#endif

  if (RuntimeLoggingEnabled() && s_frame_counter >= s_gameplay_log_frame + 300u)
  {
    s_gameplay_log_frame = s_frame_counter;
    u32 camera_state = 0xffffffffu;
    u32 ball_state = 0xffffffffu;
    u32 orbit_state = 0xffffffffu;
    float pitch = 0.0f;
    TryReadU32(guard, player + CAMERA_STATE_OFFSET, &camera_state);
    TryReadU32(guard, player + BALL_STATE_OFFSET, &ball_state);
    TryReadU32(guard, player + ORBIT_STATE_OFFSET, &orbit_state);
    TryReadFloat(guard, player + PITCH_OFFSET, &pitch);
    NOTICE_LOG_FMT(CORE,
                   "PrimedGun prime2 frame player={:08X} camera_state={} ball_state={} "
                   "orbit_state={} pitch={:.3f} move_speed={:.2f}",
                   player, camera_state, ball_state, orbit_state, pitch, s_move_speed);
    LogGunPointerScan(guard, player);
    LogGunTransformScan(guard, player);
  }
}
}  // namespace prime2
