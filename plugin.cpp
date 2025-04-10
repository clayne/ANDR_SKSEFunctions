#include <spdlog/sinks/basic_file_sink.h>
#include <warning.h>
#include "RE/N/NiPoint3.h"


namespace logger = SKSE::log;

void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::trace);
}


///// Actual new functions /////

// Papyrus: Int Function GetAndrealphusExtenderVersion() Global Native
// Returns the version number of the mod.
int GetAndrealphusExtenderVersion(RE::StaticFunctionTag*) { 
    return 150; 
}

// Papyrus: Function CastEnchantment(Actor akSource, Enchantment akEnchantment, Actor akTarget)
// Cast the akEnchantment from the akSource to the akTarget.
void CastEnchantment(RE::StaticFunctionTag*, RE::Actor* akSource, RE::EnchantmentItem* akEnchantment,
                      RE::Actor* akTarget) {   
    akSource->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)
        ->CastSpellImmediate(akEnchantment, false, akTarget, 1.0f, false, 0.0f, nullptr);
}

// Papyrus: Function CastPotion(Actor akSource, Potion akPotion, Actor akTarget)
// Cast the akPotion from the akSource to the akTarget.
void CastPotion(RE::StaticFunctionTag*, RE::Actor* akSource, RE::AlchemyItem* akPotion,
                      RE::Actor* akTarget) {
    akSource->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)
        ->CastSpellImmediate(akPotion, false, akTarget, 1.0f, false, 0.0f, nullptr);
}

// Papyrus: Function CastIngredient(Actor akSource, Ingredient akIngredient, Actor akTarget)
// Cast the akIngredient from the akSource to the akTarget.
void CastIngredient(RE::StaticFunctionTag*, RE::Actor* akSource, RE::IngredientItem* akIngredient,
                    RE::Actor* akTarget) {    
    akSource->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)
        ->CastSpellImmediate(akIngredient, false, akTarget, 1.0f, false, 0.0f, nullptr);
}

// Papyrus: Float Function GetEffectiveEnchantmentCost(Actor akSource, Enchantment akEnchantment)
// Get the total effect cost of the enchantment.
float GetEffectiveEnchantmentCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::EnchantmentItem* akEnchantment) {
    return akEnchantment->CalculateMagickaCost(akSource);
}

// Papyrus: Float Function GetEffectivePotionCost(Actor akSource, Potion akPotion)
// Get the total effect cost of the potion.
float GetEffectivePotionCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::AlchemyItem* akPotion) {
    return akPotion->CalculateMagickaCost(akSource);
}

// Papyrus: Float Function GetEffectiveIngredientCost(Actor akSource, Ingredient akIngredient)
// Get the total effect cost of the ingredient.
float GetEffectiveIngredientCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::IngredientItem* akIngredient) {
    return akIngredient->CalculateMagickaCost(akSource);
}

// Papyrus: Float Function GetEffectiveScrollCost(Actor akSource, Scroll akScroll)
// Get the total effect cost of the scroll.
float GetEffectiveScrollCost(RE::StaticFunctionTag*, RE::Actor* akSource, RE::ScrollItem* akScroll) {
    return akScroll->CalculateMagickaCost(akSource);
}

// ActiveMagicEffect Function GetActiveMagicEffectFromActor(Actor akActor, MagicEffect akMagicEffect) native global
// Get the instance of akMagicEffect on akActor
RE::ActiveEffect* GetActiveMagicEffectFromActor(RE::StaticFunctionTag*, RE::Actor* akActor,
                                                RE::EffectSetting* akMagicEffect) {
    
    if (akMagicEffect == nullptr) {
        logger::info("akMagicEffect is none.");
        return nullptr;
    }

    auto EffectsList = akActor->AsMagicTarget()->GetActiveEffectList();

    if (!EffectsList) {
        logger::info("Effects List is none.");
        return nullptr;
    }

    for (const auto& effect : *EffectsList) {
        const auto& setting = effect ? effect->GetBaseObject() : nullptr;

        if (setting) {
//            logger::info("The effect we're checking is {}. The effect we're looking for is {}.", setting->formID,
//                         akMagicEffect->formID);
            if (setting == akMagicEffect) {
                return effect;
            }
        } else {
            logger::info("The setting is nullptr");
        }
    }
    logger::info("Effect is none.");
    return nullptr;
}

void SetRefAsNoAIAcquire(RE::StaticFunctionTag*, RE::TESObjectREFR* akObject, bool SetNoAIAquire) {
    if (SetNoAIAquire == true) {
        akObject->formFlags |= RE::TESObjectREFR::RecordFlags::RecordFlag::kNoAIAcquire;
    } else {
        akObject->formFlags &= ~RE::TESObjectREFR::RecordFlags::RecordFlag::kNoAIAcquire;
    }
}

// stuff for LaunchAmmo's 3rd attempt

/*
RE::NiPoint3 GetEyePosition(RE::Actor* actor) {
    float eyeHeight = actor->GetHeight() * 0.9f;
    auto pos = actor->GetPosition();
    pos.z += eyeHeight;
    return pos;
}

struct LaunchParams {
    RE::NiPoint3 origin;
    float pitch;
    float yaw;
};

LaunchParams GetLaunchOriginAndAngles(RE::Actor* actor, RE::NiAVObject* fireNode, RE::Actor* CombatTarget) {
    LaunchParams result{};

    auto origin = fireNode->world.translate;

    RE::NiPoint3 forward;

    if (actor->IsPlayerRef()) {
        auto playerCam = RE::PlayerCamera::GetSingleton();

        if (playerCam && playerCam->cameraRoot) {
            auto cameraMatrix = playerCam->cameraRoot->world.rotate;
            forward = cameraMatrix * RE::NiPoint3(0.0f, 1.0f, 0.0f);
        } else {
            float pitch = actor->GetAngleX();
            float yaw = actor->GetAngleZ();

            forward.x = std::cos(pitch) * std::cos(yaw);
            forward.y = std::cos(pitch) * std::sin(yaw);
            forward.z = std::sin(pitch);
        }

    } else {
        if (auto target = CombatTarget) {
            auto targetPos = GetEyePosition(target);
            forward.x = targetPos.x - origin.x;
            forward.y = targetPos.y - origin.y;
            forward.z = targetPos.z - origin.z;
            forward.Unitize();
        } else {
            float pitch = actor->GetAngleX();
            float yaw = actor->GetAngleZ();

            forward.x = std::cos(pitch) * std::cos(yaw);
            forward.y = std::cos(pitch) * std::sin(yaw);
            forward.z = std::sin(pitch);
        }
    }

    result.pitch = std::asin(forward.z); 
    result.yaw = std::atan2(forward.y, forward.x);

    return result;
}
*/

inline void LaunchAmmo(RE::StaticFunctionTag*, RE::Actor* a_actor, RE::TESAmmo* a_ammo, RE::TESObjectWEAP* a_weapon,
                        RE::BSFixedString a_nodeName, std::int32_t a_source, RE::TESObjectREFR* a_target,
                        RE::AlchemyItem* a_poison, RE::BGSProjectile* ProjBase, RE::Actor* a_combattarget) {
 
//   ProjBase needs to be assigned through Papyrus. Using a_ammo->data.projectile gives an invalid value. As does getting it through launchData.

     SKSE::GetTaskInterface()->AddTask([a_actor, a_ammo, a_weapon, a_nodeName, a_source, a_target, a_poison, ProjBase,
                                       a_combattarget]() {
         RE::NiAVObject* fireNode = nullptr;
         auto root = a_actor->GetCurrent3D();
         switch (a_source) {
             case -1: {
                 if (!a_nodeName.empty()) {
                     if (root) {
                         fireNode = root->GetObjectByName(a_nodeName);
                     }
                 } else {
                     if (const auto currentProcess = a_actor->GetActorRuntimeData().currentProcess) {
                         const auto& biped = a_actor->GetBiped2();
                         fireNode = a_weapon->IsCrossbow() ? currentProcess->GetMagicNode(biped)
                                                           : currentProcess->GetWeaponNode(biped);
                     } else {
                         fireNode = a_weapon->GetFireNode(root);
                     }
                 }
             } break;
             case 0:
                 fireNode = root ? root->GetObjectByName(RE::FixedStrings::GetSingleton()->npcLMagicNode) : nullptr;
                 break;
             case 1:
                 fireNode = root ? root->GetObjectByName(RE::FixedStrings::GetSingleton()->npcRMagicNode) : nullptr;
                 break;
             case 2:
                 fireNode = root ? root->GetObjectByName(RE::FixedStrings::GetSingleton()->npcHeadMagicNode) : nullptr;
                 break;
             default:
                 break;
         }
         RE::NiPoint3 origin;
         RE::Projectile::ProjectileRot angles{};

//      First attempt -> works in 1st person, but aim is off in 3rd person and for NPCs.
         if (fireNode) {
             origin = fireNode->world.translate;

//           This line didsn't work in 3rd person or for NPCs... -> aim is completely off (shoots up to the sky).
//           a_actor->Unk_A0(fireNode, angles.x, angles.z, origin);             

             angles.x = a_actor->GetAimAngle();
             angles.z = a_actor->GetAimHeading();
         } else {
             origin = a_actor->GetPosition();
             origin.z += 96.0f;

             angles.x = a_actor->GetAimAngle();
             angles.z = a_actor->GetAimHeading();
         }

//      Second attempt -> didn't work: 1st person was fine, 3rd person and NPCs were completely off. (worse than 1st attempt)
/* auto camera = RE::PlayerCamera::GetSingleton();

         if (a_actor->IsPlayerRef()) {
                      
             if (camera->IsInFirstPerson()) {

                 if (fireNode) {
                     origin = fireNode->world.translate;
     //             a_actor->Unk_A0(fireNode, angles.x, angles.z, origin);

                     angles.x = a_actor->GetAimAngle();
                     angles.z = a_actor->GetAimHeading();
                 } else {
                     origin = a_actor->GetPosition();
                     origin.z += 96.0f;

                     angles.x = a_actor->GetAimAngle();
                     angles.z = a_actor->GetAimHeading();
                 }

             } else {

                 if (fireNode) {
                    origin = fireNode->world.translate;

                    RE::NiPoint3 forward = fireNode->world.rotate * RE::NiPoint3{0, 1, 0};    // Y-axis is forward
                    angles.x = std::asin(forward.z);                                        // pitch
                    angles.z = std::atan2(forward.y, forward.x);                              // yaw
                 }
             }
             
         } else {
         // For NPCs
             if (fireNode) {
                 origin = fireNode->world.translate;
                 RE::NiPoint3 direction;
                 
                 if (a_combattarget) {
                    RE::NiPoint3 targetPos = a_combattarget->GetPosition();
                    RE::NiPoint3 delta = targetPos - origin;
                    direction = delta / delta.Length();  // normalize

                    angles.x = std::asin(direction.z);              // pitch
                    angles.z = std::atan2(direction.y, direction.x);  // yaw
                 } else {
                    // fallback: fire straight ahead from actor rotation
                    float actorAngleZ = a_actor->data.angle.z;
                    angles.x = 0.0f;
                    angles.z = actorAngleZ;
                 }

             } else {
                 origin = a_actor->GetPosition();
                 origin.z += 96.0f;

                 angles.x = a_actor->GetAimAngle();
                 angles.z = a_actor->GetAimHeading();
             }  

         }
*/

//       3rd attempt -> didn't work: 1st person, 3rd person and NPCs were completely off. (worse than 1st and 2nd attempt)
//       auto params = GetLaunchOriginAndAngles(a_actor, fireNode, a_combattarget);

        if (fireNode) {
             logger::info("FireNode found: {}", fireNode->name.c_str());
             logger::info("Origin: x={}, y={}, z={}", origin.x, origin.y, origin.z);
             logger::info("Angles: pitch(x)={}, yaw(z)={}", angles.x, angles.z);
         } else {
             logger::info("FireNode is null; using actor position: x={}, y={}, z={}", origin.x, origin.y, origin.z);
             logger::info("Aim angles: pitch(x)={}, yaw(z)={}", angles.x, angles.z);
         }

         RE::ProjectileHandle handle{};
         RE::Projectile::LaunchData launchData(a_actor, origin, angles, a_ammo, a_weapon);

         if (a_target) { 
             launchData.desiredTarget = a_target; 
         }
         if (a_poison) {
             launchData.poison = a_poison;        
         }
         
//       launchData.enchantItem = a_weapon->formEnchanting;
         launchData.autoAim = false;
         launchData.projectileBase = ProjBase;

//      3rd attempt -> see above!
 //        launchData.angleX = params.pitch;
 //        launchData.angleZ = params.yaw;

         RE::Projectile::Launch(&handle, launchData);
     });
}

bool PapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("GetAndrealphusExtenderVersion", "ANDR_PapyrusFunctions", GetAndrealphusExtenderVersion);
    vm->RegisterFunction("CastEnchantment", "ANDR_PapyrusFunctions", CastEnchantment);
    vm->RegisterFunction("CastPotion", "ANDR_PapyrusFunctions", CastPotion);
    vm->RegisterFunction("CastIngredient", "ANDR_PapyrusFunctions", CastIngredient);
    vm->RegisterFunction("GetEffectiveEnchantmentCost", "ANDR_PapyrusFunctions", GetEffectiveEnchantmentCost);
    vm->RegisterFunction("GetEffectivePotionCost", "ANDR_PapyrusFunctions", GetEffectivePotionCost);
    vm->RegisterFunction("GetEffectiveIngredientCost", "ANDR_PapyrusFunctions", GetEffectiveIngredientCost);
    vm->RegisterFunction("GetEffectiveScrollCost", "ANDR_PapyrusFunctions", GetEffectiveScrollCost);
    vm->RegisterFunction("GetActiveMagicEffectFromActor", "ANDR_PapyrusFunctions", GetActiveMagicEffectFromActor);
    vm->RegisterFunction("SetRefAsNoAIAcquire", "ANDR_PapyrusFunctions", SetRefAsNoAIAcquire);
    vm->RegisterFunction("LaunchAmmo", "ANDR_PapyrusFunctions", LaunchAmmo);    
/*  depreciated functions
    vm->RegisterFunction("CastSpellFromRef", "ANDR_PapyrusFunctions", CastSpellFromRef);
    vm->RegisterFunction("CastSpellFromPointToPoint", "ANDR_PapyrusFunctions", CastSpellFromPointToPoint);
    vm->RegisterFunction("CastSpellFromHand", "ANDR_PapyrusFunctions", CastSpellFromHand);
*/
    return true;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SetupLog();
    SKSE::GetPapyrusInterface()->Register(PapyrusFunctions);
    return true;
}

/* 
    Old depreciated code -> kept as backup and reference.
    
    Instructions from Fenix to get the position and rotation for CastSpellFromHand() function.
    
    struct ProjectileRot {
        float x, z;
    };

    typedef uint32_t (*cast_t)(RE::Actor* caster, RE::SpellItem* spel, const RE::NiPoint3& start_pos,
                               const ProjectileRot& rot);
    typedef uint32_t (*cast_CustomPos_t)(RE::Actor* caster, RE::SpellItem* spel, const RE::NiPoint3& start_pos,
                                         const ProjectileRot& rot);

    RE::EffectSetting* getAVEffectSetting(RE::MagicItem* mgitem) {
        using func_t = decltype(getAVEffectSetting);
        REL::Relocation<func_t> func{REL::RelocationID(11194, 11302)};
        return func(mgitem);
    }

    float SkyrimSE_c51f70(RE::NiPoint3* dir) {
        using func_t = decltype(SkyrimSE_c51f70);
        REL::Relocation<func_t> func{REL::RelocationID(68820, 70172)};
        return func(dir);
    }

    auto rot_at(RE::NiPoint3 dir) {
        ProjectileRot rot;
        auto len = dir.Unitize();
        if (len == 0) {
            rot = {0, 0};
        } else {
            float polar_angle = SkyrimSE_c51f70(&dir);
            rot = {-asin(dir.z), polar_angle};
        }
        return rot;
    }

    auto rot_at(const RE::NiPoint3& from, const RE::NiPoint3& to) { return rot_at(to - from); }

    // Papyrus: Function CastSpellFromHand(Actor akSource, Spell akSpell, ObjectReference akTarget, ObjectReference
    akOriginRef) global native
    // Cast a spell from the hand defined in PositionInt at the akTarget.
    void CastSpellFromRef(RE::StaticFunctionTag*, RE::Actor* akSource, RE::SpellItem* akSpell, RE::TESObjectREFR*
    akTarget, RE::TESObjectREFR* akOriginRef) {

    //    auto NodePosition = akSource
    //                            ->GetMagicCaster(PositionInt == 0 ? RE::MagicSystem::CastingSource::kLeftHand
    //                                                              : RE::MagicSystem::CastingSource::kRightHand)
    //                            ->GetMagicNode()
    //                            ->world.translate;
        auto NodePosition = akOriginRef->GetPosition();

        logger::info("Position: X is {}, Y is {}, Z is {}.", NodePosition.x, NodePosition.y, NodePosition.z);

        auto rot = rot_at(NodePosition, akTarget->GetPosition());

        logger::info("Rotation: X is {}, Z is {}.", rot.x, rot.z);

        auto eff = akSpell->GetCostliestEffectItem();

        auto mgef = getAVEffectSetting(akSpell);

        RE::Projectile::LaunchData ldata;

        ldata.origin = NodePosition;
        ldata.contactNormal = {0.0f, 0.0f, 0.0f};
        ldata.projectileBase = mgef->data.projectileBase;
        ldata.shooter = akSource;
        ldata.combatController = akSource->GetActorRuntimeData().combatController;
        ldata.weaponSource = nullptr;
        ldata.ammoSource = nullptr;
        ldata.angleZ = rot.z;
        ldata.angleX = rot.x;
        ldata.unk50 = nullptr;
        ldata.desiredTarget = nullptr;
        ldata.unk60 = 0.0f;
        ldata.unk64 = 0.0f;
        ldata.parentCell = akSource->GetParentCell();
        ldata.spell = akSpell;
        ldata.castingSource = RE::MagicSystem::CastingSource::kOther;
    //    ldata.unk7C = 0;
        ldata.enchantItem = nullptr;
        ldata.poison = nullptr;
        ldata.area = eff->GetArea();
        ldata.power = 1.0f;
        ldata.scale = 1.0f;
        ldata.alwaysHit = false;
        ldata.noDamageOutsideCombat = false;
        ldata.autoAim = false;
    //    ldata.unk9F = false;
        ldata.useOrigin = true;
        ldata.deferInitialization = false;
        ldata.forceConeOfFire = false;

        RE::BSPointerHandle<RE::Projectile> handle;
        RE::Projectile::Launch(&handle, ldata);
    }

    // Function CastSpellFromPointToPoint(Actor akSource, Spell akSpell, Float StartPoint_X, Float StartPoint_Y,
    //                                   Float StartPoint_Z, Float EndPoint_X, Float EndPoint_Y,
    //                                   Float EndPoint_Z) native global {

    void CastSpellFromPointToPoint(RE::StaticFunctionTag*, RE::Actor* akSource, RE::SpellItem* akSpell, float
    StartPoint_X, float StartPoint_Y, float StartPoint_Z, float EndPoint_X, float EndPoint_Y, float EndPoint_Z) {

         RE::NiPoint3 NodePosition;

         NodePosition.x = StartPoint_X;
         NodePosition.y = StartPoint_Y;
         NodePosition.z = StartPoint_Z;

         logger::info("NodePosition: X = {}, Y = {}, Z = {}.", NodePosition.x, NodePosition.y, NodePosition.z);

         RE::NiPoint3 DestinationPosition;

         DestinationPosition.x = EndPoint_X;
         DestinationPosition.y = EndPoint_Y;
         DestinationPosition.z = EndPoint_Z;

         logger::info("DestinationPosition: X = {}, Y = {}, Z = {}.", DestinationPosition.x, DestinationPosition.y,
                      DestinationPosition.z);

         auto rot = rot_at(NodePosition, DestinationPosition);

         auto eff = akSpell->GetCostliestEffectItem();

         auto mgef = getAVEffectSetting(akSpell);

         RE::Projectile::LaunchData ldata;

         ldata.origin = NodePosition;
         ldata.contactNormal = {0.0f, 0.0f, 0.0f};
         ldata.projectileBase = mgef->data.projectileBase;
         ldata.shooter = akSource;
         ldata.combatController = akSource->GetActorRuntimeData().combatController;
         ldata.weaponSource = nullptr;
         ldata.ammoSource = nullptr;
         ldata.angleZ = rot.z;
         ldata.angleX = rot.x;
         ldata.unk50 = nullptr;
         ldata.desiredTarget = nullptr;
         ldata.unk60 = 0.0f;
         ldata.unk64 = 0.0f;
         ldata.parentCell = akSource->GetParentCell();
         ldata.spell = akSpell;
         ldata.castingSource = RE::MagicSystem::CastingSource::kOther;
    //     ldata.unk7C = 0;
         ldata.enchantItem = nullptr;
         ldata.poison = nullptr;
         ldata.area = eff->GetArea();
         ldata.power = 1.0f;
         ldata.scale = 1.0f;
         ldata.alwaysHit = false;
         ldata.noDamageOutsideCombat = false;
         ldata.autoAim = false;
    //     ldata.unk9F = false;
         ldata.useOrigin = true;
         ldata.deferInitialization = false;
         ldata.forceConeOfFire = false;

         RE::BSPointerHandle<RE::Projectile> handle;
         RE::Projectile::Launch(&handle, ldata);

    }


    // Math for this function seems to be different than through papyrus, as such it's done as a non-native function,
    that calls CastSpellFromPointToPoint() for now.
    // Papyrus: Function CastSpellFromHand(Actor akSource, Spell akSpell, Bool IsLeftHand, Float DistanceVar = 2000.0,
    Float HeightVar = 100.0,
    // Bool UseCustomObject = False, Form akObjectBase = None,
    // Float Offset_NoSneak_Left_X = 30.0, Float Offset_NoSneak_Left_Y = 30.0, Float Offset_NoSneak_Left_Z = 110.0,
    // Float Offset_NoSneak_Right_X = 30.0, Float Offset_NoSneak_Right_Y = -30.0, Float Offset_NoSneak_Right_Z = 110.0,
    // Float Offset_Sneak_Left_X = 30.0, Float Offset_Sneak_Left_Y = 30.0, Float Offset_Sneak_Left_Z = 70.0,
    // Float Offset_Sneak_Right_X = 30.0, Float Offset_Sneak_Right_Y = -30.0, Float Offset_Sneak_Right_Z = 70.0)
    void CastSpellFromHand(RE::StaticFunctionTag*, RE::Actor* akSource, RE::SpellItem* akSpell, bool IsLeftHand,
                             float DistanceVar, float HeightVar, float Offset_NoSneak_Left_X, float
    Offset_NoSneak_Left_Y, float Offset_NoSneak_Left_Z, float Offset_NoSneak_Right_X, float Offset_NoSneak_Right_Y,
                             float Offset_NoSneak_Right_Z, float Offset_Sneak_Left_X, float Offset_Sneak_Left_Y,
                             float Offset_Sneak_Left_Z, float Offset_Sneak_Right_X, float Offset_Sneak_Right_Y,
                             float Offset_Sneak_Right_Z) {

          float GameX = akSource->GetAngle().x;
          float GameZ = akSource->GetAngle().z;
          float AngleX = 90.0 + GameX;
          float AngleZ;

          float SourceMarkerXOffset_Standard;
          float SourceMarkerYOffset_Standard;
          float SourceMarkerZOffset_Standard;

          if (GameZ < 90.0) {
              AngleZ = (90.0 - GameZ);
          } else {
              AngleZ = (450.0 - GameZ);
          }

        if (akSource->IsSneaking())
          {
                  if (IsLeftHand == true) {
                      SourceMarkerXOffset_Standard = Offset_Sneak_Left_X;
                      SourceMarkerYOffset_Standard = Offset_Sneak_Left_Y;
                      SourceMarkerZOffset_Standard = Offset_Sneak_Left_Z;
                  } else {
                      SourceMarkerXOffset_Standard = Offset_Sneak_Right_X;
                      SourceMarkerYOffset_Standard = Offset_Sneak_Right_Y;
                      SourceMarkerZOffset_Standard = Offset_Sneak_Right_Z;
                  }

          } else {
                  if (IsLeftHand == true) {
                          SourceMarkerXOffset_Standard = Offset_NoSneak_Left_X;
                          SourceMarkerYOffset_Standard = Offset_NoSneak_Left_Y;
                          SourceMarkerZOffset_Standard = Offset_NoSneak_Left_Z;
                  } else {
                          SourceMarkerXOffset_Standard = Offset_NoSneak_Right_X;
                          SourceMarkerYOffset_Standard = Offset_NoSneak_Right_Y;
                          SourceMarkerZOffset_Standard = Offset_NoSneak_Right_Z;
                  }
          }

          RE::NiPoint3 NodePosition;

          NodePosition.x = (akSource->GetPositionX() + (cos(AngleZ) * SourceMarkerXOffset_Standard - sin(AngleZ) *
          SourceMarkerYOffset_Standard));
          NodePosition.y = (akSource->GetPositionY() + (cos(AngleZ) * SourceMarkerYOffset_Standard + sin(AngleZ) *
          SourceMarkerXOffset_Standard));
          NodePosition.z = (akSource->GetPositionZ() + SourceMarkerZOffset_Standard);

          logger::info("NodePosition: X = {}, Y = {}, Z = {}.", NodePosition.x, NodePosition.y, NodePosition.z);

          RE::NiPoint3 DestinationPosition;

          DestinationPosition.x = (akSource->GetPositionX() + (DistanceVar * sin(AngleX) * cos(AngleZ)));
          DestinationPosition.y = (akSource->GetPositionY() + (DistanceVar * sin(AngleX) * sin(AngleZ)));
          DestinationPosition.z = (akSource->GetPositionZ() + (DistanceVar * cos(AngleX) + HeightVar));

          logger::info("DestinationPosition: X = {}, Y = {}, Z = {}.", DestinationPosition.x, DestinationPosition.y,
                       DestinationPosition.z);

          auto rot = rot_at(NodePosition, DestinationPosition);

          auto eff = akSpell->GetCostliestEffectItem();

          auto mgef = getAVEffectSetting(akSpell);

          RE::Projectile::LaunchData ldata;

          ldata.origin = NodePosition;
          ldata.contactNormal = {0.0f, 0.0f, 0.0f};
          ldata.projectileBase = mgef->data.projectileBase;
          ldata.shooter = akSource;
          ldata.combatController = akSource->GetActorRuntimeData().combatController;
          ldata.weaponSource = nullptr;
          ldata.ammoSource = nullptr;
          ldata.angleZ = rot.z;
          ldata.angleX = rot.x;
          ldata.unk50 = nullptr;
          ldata.desiredTarget = nullptr;
          ldata.unk60 = 0.0f;
          ldata.unk64 = 0.0f;
          ldata.parentCell = akSource->GetParentCell();
          ldata.spell = akSpell;
          ldata.castingSource = RE::MagicSystem::CastingSource::kOther;
          ldata.unk7C = 0;
          ldata.enchantItem = nullptr;
          ldata.poison = nullptr;
          ldata.area = eff->GetArea();
          ldata.power = 1.0f;
          ldata.scale = 1.0f;
          ldata.alwaysHit = false;
          ldata.noDamageOutsideCombat = false;
          ldata.autoAim = false;
          ldata.unk9F = false;
          ldata.useOrigin = true;
          ldata.deferInitialization = false;
          ldata.forceConeOfFire = false;

          RE::BSPointerHandle<RE::Projectile> handle;
          RE::Projectile::Launch(&handle, ldata);
    }

    // This line is needed for CastSpellFromRef(), CastSpellFromHand() and CastSpellFromPointToPoint() to compile, might
    no longer be needed in the future? ---> This is a dtor.

    RE::Projectile::LaunchData::~LaunchData() {}
 */