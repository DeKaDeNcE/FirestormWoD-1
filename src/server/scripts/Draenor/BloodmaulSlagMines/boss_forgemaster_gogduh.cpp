#include "instance_bloodmaul.h"

namespace MS
{
    namespace Instances
    {
        namespace Bloodmaul
        {
            class boss_forgemaster_gogduh : public CreatureScript
            {
            public:
                boss_forgemaster_gogduh() : CreatureScript("boss_forgemaster_gogduh")
                {
                }

                enum class Spells
                {
                    MagmaBarrage = 150004,
                };

                enum class Events
                {
                    MagmaBarrage    = 1,
                    SpawnRuination  = 2,
                    SpawnCalamity   = 3,
                };

                enum class NPCs
                {
                    Ruination   = 74570,
                    Calamity    = 74571   
                };

                CreatureAI* GetAI(Creature* creature) const
                {
                    return new boss_AI(creature);
                }

                struct boss_AI : public BossAI
                {
                    boss_AI(Creature* p_Creature) : BossAI(p_Creature, uint32(BossIds::ForgemasterGogduh))
                    {
                        if (instance)
                            instance->SetBossState(uint32(BossIds::ForgemasterGogduh), EncounterState::TO_BE_DECIDED);
                    }

                    Position const m_RuinationSpawnPos = Position{2093.f, 113.f, 225.f, 4.583f};
                    Position const m_CalamitySpawnPos  = Position{2071.f, 111.f, 225.f, 5.003f};
                    
                    void DoCast(Unit* p_UnitTarget, Spells p_SpellId, bool p_Triggered)
                    {
                        me->CastSpell(p_UnitTarget, (uint32)p_SpellId, p_Triggered);
                    }
                    
                    void DoCast(float p_X, float p_Y, float p_Z, Spells p_SpellId, bool p_Triggered)
                    {
                        me->CastSpell(p_X, p_Y, p_Z, (uint32)p_SpellId, p_Triggered);
                    }

                    void Reset()
                    {
                        DespawnAllSummons();
                        _Reset();
                    }

                    void JustReachedHome()
                    {
                        _JustReachedHome();

                        if (instance)
                        {
                            instance->SetBossState(uint32(BossIds::ForgemasterGogduh), FAIL);
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                        }
                    }

                    void JustDied(Unit* /*killer*/)
                    {
                        DespawnAllSummons();
                        _JustDied();
                    }

                    void KilledUnit(Unit* /*victim*/)
                    {
                    }

                    void EnterCombat(Unit* who)
                    {
                        _EnterCombat();

                        if (instance)
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                        events.ScheduleEvent((uint32)Events::MagmaBarrage, 5000);
                        events.ScheduleEvent((uint32)Events::SpawnRuination, 15000);
                        events.ScheduleEvent((uint32)Events::SpawnCalamity, 30000);
                    }

                    void UpdateAI(const uint32 diff)
                    {
                        if (!UpdateVictim())
                            return;

                        events.Update(diff);

                        switch ((Events)events.ExecuteEvent())
                        {
                            case Events::MagmaBarrage:
                                // HACK - needs to be cleared, channeling system is a bit fucked up xD
                                me->GetMotionMaster()->Clear();
                                DoCast(me, Spells::MagmaBarrage, false);
                                break;
                            case Events::SpawnRuination:
                                if (Unit* l_Summon = SummonCreature(NPCs::Ruination, m_RuinationSpawnPos))
                                    l_Summon->Attack(me->getVictim(), true);
                                break;
                            case Events::SpawnCalamity:
                                if (Unit* l_Summon = SummonCreature(NPCs::Calamity, m_CalamitySpawnPos))
                                    l_Summon->Attack(me->getVictim(), true);
                                break;
                            default:
                                break;
                        }

                        if (me->HasUnitState(UNIT_STATE_CASTING) || me->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                            return;

                        DoMeleeAttackIfReady();
                    }

                    Unit* SummonCreature(NPCs p_Creature, Position const& p_Position)
                    {
                        if (TempSummon* l_Summon = me->SummonCreature((uint32)p_Creature, p_Position, TEMPSUMMON_DEAD_DESPAWN))
                        {
                            m_SpawnedCreatures.push_back(l_Summon->GetGUID());
                            return l_Summon;
                        }

                        return nullptr;
                    }

                    void DespawnAllSummons()
                    {
                        for (auto& l_GUID : m_SpawnedCreatures)
                            if (Unit* l_Spawn = sObjectAccessor->GetCreature(*me, l_GUID))
                                if (TempSummon* l_TempSummon = l_Spawn->ToTempSummon())
                                    l_TempSummon->UnSummon();
                    }

                    std::list<uint64> m_SpawnedCreatures;
                };
            };

            class npc_ruination : public CreatureScript
            {
            public:
                npc_ruination() : CreatureScript("npc_ruination")
                {
                }

                enum class Spells
                {
                    ShatterEarth    = 150324,
                    EarthSmash      = 149941
                };

                enum class Events
                {
                    CastSpell = 1
                };

                CreatureAI* GetAI(Creature* creature) const
                {
                    return new npc_ruinationAI(creature);
                }

                struct npc_ruinationAI : public ScriptedAI
                {
                    npc_ruinationAI(Creature* p_Creature) : ScriptedAI(p_Creature)
                    {
                    }

                    void Reset()
                    {
                        events.Reset();
                        events.ScheduleEvent((uint32)Events::CastSpell, 6000);
                    }
                    
                    void DoCast(Unit* p_UnitTarget, Spells p_SpellId, bool p_Triggered)
                    {
                        me->CastSpell(p_UnitTarget, (uint32)p_SpellId, p_Triggered);
                    }

                    void UpdateAI(const uint32 diff)
                    {
                        if (!UpdateVictim())
                            return;
                            
                        if (me->HasUnitState(UNIT_STATE_CASTING) || me->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                            return;

                        events.Update(diff);

                        if ((Events)events.ExecuteEvent() == Events::CastSpell)
                        {
                            DoCast(me->getVictim(), urand(0, 1) ? Spells::ShatterEarth : Spells::EarthSmash, false);
                            events.ScheduleEvent((uint32)Events::CastSpell, 5000);
                        }

                        DoMeleeAttackIfReady();
                    }

                };
            };

            class npc_calamity : public CreatureScript
            {
            public:
                npc_calamity() : CreatureScript("npc_calamity")
                {
                }

                enum class Spells
                {
                    DancingFlames   = 149975,
                    Firestorm       = 144461,
                    Scorch          = 150290
                };

                enum class Events
                {
                    CastSpell   = 1
                };

                CreatureAI* GetAI(Creature* creature) const
                {
                    return new npc_calamityAI(creature);
                }

                struct npc_calamityAI : public ScriptedAI
                {
                    npc_calamityAI(Creature* p_Creature) : ScriptedAI(p_Creature)
                    {
                    }

                    void Reset()
                    {
                        events.Reset();
                        events.ScheduleEvent((uint32)Events::CastSpell, 12000);
                    }

                    void UpdateAI(const uint32 diff)
                    {
                        if (!UpdateVictim())
                            return;

                        events.Update(diff);
                        
                        if (me->HasUnitState(UNIT_STATE_CASTING) || me->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                            return;

                        switch ((Events)events.ExecuteEvent())
                        {
                            case Events::CastSpell:
                                me->CastSpell(me, uint32(urand(0, 1) ? Spells::DancingFlames : Spells::Scorch), false);
                                events.ScheduleEvent((uint32)Events::CastSpell, 12000);
                            default:
                                return;
                        }

                        me->CastSpell(me->getVictim(), (uint32)Spells::Scorch, true);

                        if (me->HasUnitState(UNIT_STATE_CASTING) || me->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                            return;

                        DoMeleeAttackIfReady();
                    }

                };
            };

            class spell_rough_smash: public SpellScriptLoader
            {
            public:
                spell_rough_smash() : SpellScriptLoader("spell_rough_smash") { }

                class spell_rough_smash_SpellScript : public SpellScript
                {
                    PrepareSpellScript(spell_rough_smash_SpellScript);
            
                    class RoughSmashTargetFilter
                    {
                    public:
                        explicit RoughSmashTargetFilter(Position* p_CasterPosition) : m_CasterPosition(p_CasterPosition) { }

                        bool operator()(WorldObject* p_Unit) const
                        {
                            return !m_CasterPosition->HasInArc(float(M_PI) / 3.f, p_Unit);
                        }

                    private:
                        Position* m_CasterPosition;
                    };

                    void FilterTargets(std::list<WorldObject*>& targets)
                    {
                        targets.remove_if(RoughSmashTargetFilter(&m_CastPosition));
                    }

                    void HandleOnPrepare()
                    {
                        GetCaster()->GetPosition(&m_CastPosition);
                    }

                    Position m_CastPosition;

                    void Register()
                    {
                        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rough_smash_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
                        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rough_smash_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_CONE_ENEMY_104);
                        OnPrepare += SpellOnPrepareFn(spell_rough_smash_SpellScript::HandleOnPrepare);
                    }
                };
            };

            class spell_shatter_earth: public SpellScriptLoader
            {
            public:
                spell_shatter_earth() : SpellScriptLoader("spell_shatter_earth") { }
                
                enum Spells
                {
                    ShatterEarthDamage = 150344,
                    ShatterEarthNE     = 149959,
                    ShatterEarthNW     = 149968,
                    ShatterEarthSW     = 149970,
                    ShatterEarthSE     = 149969
                };

                class spell_shatter_earth_SpellScript : public SpellScript
                {
                    PrepareSpellScript(spell_shatter_earth_SpellScript);

                    void OnSpellHit()
                    {
                        uint32 const ShatterEarthSpikeSpells[] {(uint32)Spells::ShatterEarthNE, (uint32)Spells::ShatterEarthNW, (uint32)Spells::ShatterEarthSW, (uint32)Spells::ShatterEarthSE } ;

                        if (Unit* l_Target = GetCaster()->SelectNearbyTarget(nullptr, VISIBLE_RANGE))
                            for (int l_I = 0; l_I < 4; l_I++)
                                GetCaster()->CastSpell(l_Target->GetPositionX(), l_Target->GetPositionY(), l_Target->GetPositionZ(), ShatterEarthSpikeSpells[l_I], true);

                        GetCaster()->CastSpell(GetCaster(), (uint32)Spells::ShatterEarthDamage, true);
                    }

                    void Register()
                    {
                        OnHit += SpellHitFn(spell_shatter_earth_SpellScript::OnSpellHit);
                    }
                };

                SpellScript* GetSpellScript() const
                {
                    return new spell_shatter_earth_SpellScript();
                }
            };

            class areatrigger_shatter_earth : public AreaTriggerEntityScript
            {
                enum Spells
                {
                    ShatterEarthNE     = 149959,
                    ShatterEarthNW     = 149968,
                    ShatterEarthSW     = 149970,
                    ShatterEarthSE     = 149969,
                    ShatterEarthSpawn  = 150325,
                    ShatterEarthDamage = 149963
                };

            public:
                areatrigger_shatter_earth() : AreaTriggerEntityScript("areatrigger_shatter_earth")
                {
                }
        
                void OnSetCreatePosition(AreaTrigger* p_AreaTrigger, Unit* p_Caster, Position& p_SourcePosition, Position& p_DestinationPosition)
                {
                    if (!p_Caster)
                        return;

                    float l_Orientation;
                    switch ((Spells)p_AreaTrigger->GetSpellId())
                    {
                        case Spells::ShatterEarthNW:
                            l_Orientation = M_PI / 4.f; // 45�
                            break;
                        case Spells::ShatterEarthSW:
                            l_Orientation = M_PI / 4.f * 3.f; // 135�
                            break;
                        case  Spells::ShatterEarthSE:
                            l_Orientation = M_PI / 4.f * 5.f; // 225�
                            break;
                        case Spells::ShatterEarthNE:
                            l_Orientation = M_PI / 4.f * 7.f; // 315�
                            break;
                        default:
                            l_Orientation = 0.f;
                    }
                    
                    p_SourcePosition.m_positionX = p_SourcePosition.GetPositionX() + (cos(l_Orientation) * 8.f);
                    p_SourcePosition.m_positionY = p_SourcePosition.GetPositionY() + (sin(l_Orientation) * 8.f);
                    p_SourcePosition.m_positionZ = p_SourcePosition.GetPositionZ();
                    p_Caster->UpdateGroundPositionZ(p_SourcePosition.m_positionX, p_SourcePosition.m_positionY, p_SourcePosition.m_positionZ);
                    p_SourcePosition.SetOrientation(l_Orientation);

                    p_DestinationPosition.m_positionX = p_SourcePosition.GetPositionX() + (cos(l_Orientation) * 20.f);
                    p_DestinationPosition.m_positionY = p_SourcePosition.GetPositionY() + (sin(l_Orientation) * 20.f);
                    p_DestinationPosition.m_positionZ = p_SourcePosition.GetPositionZ();
                    p_Caster->UpdateGroundPositionZ(p_DestinationPosition.m_positionX, p_DestinationPosition.m_positionY, p_DestinationPosition.m_positionZ);
                    p_DestinationPosition.SetOrientation(l_Orientation);
                }

                void OnUpdate(AreaTrigger* p_AreaTrigger, uint32 p_Time)
                {
                    Unit* l_Caster = p_AreaTrigger->GetCaster();
                    std::list<Unit*> l_TargetList;
                    float l_Radius = 3.f;

                    if (!l_Caster)
                        return;

                    JadeCore::NearestAttackableUnitInObjectRangeCheck u_check(p_AreaTrigger, l_Caster, l_Radius);
                    JadeCore::UnitListSearcher<JadeCore::NearestAttackableUnitInObjectRangeCheck> searcher(p_AreaTrigger, l_TargetList, u_check);
                    p_AreaTrigger->VisitNearbyObject(l_Radius, searcher);
                    bool l_HasTarget = false;

                    for (auto& l_Itr : l_TargetList)
                    {
                        if (!l_Itr->HasAura((uint32)Spells::ShatterEarthDamage))
                        {
                            l_HasTarget = true;
                            break;
                        }
                    }

                    if (l_HasTarget)
                        l_Caster->CastSpell(p_AreaTrigger->GetPositionX(), p_AreaTrigger->GetPositionY(), p_AreaTrigger->GetPositionZ(), (uint32)Spells::ShatterEarthSpawn, true);
                }

                AreaTriggerEntityScript* GetAI() const
                {
                    return new areatrigger_shatter_earth();
                }
            };

            class npc_shatter_earth : public CreatureScript
            {
            public:
                npc_shatter_earth() : CreatureScript("npc_shatter_earth")
                {
                }

                enum class Spells
                {
                    ShatterEarthDamage  = 149963
                };


                CreatureAI* GetAI(Creature* creature) const
                {
                    return new npc_shatter_earthAI(creature);
                }

                struct npc_shatter_earthAI : public ScriptedAI
                {
                    npc_shatter_earthAI(Creature* p_Creature) : ScriptedAI(p_Creature)
                    {
                    }

                    void Reset()
                    {
                        if (Unit* l_Target = me->SelectNearbyTarget(nullptr, 4.f, (uint32)Spells::ShatterEarthDamage))
                            me->CastSpell(l_Target, (uint32)Spells::ShatterEarthDamage, true);
                        m_DeathTimer = 2000;
                    }

                    void UpdateAI(const uint32 p_Diff)
                    {
                        if (me->isDead())
                            return;

                        if (m_DeathTimer <= 0)
                        {
                            me->DealDamage(me, me->GetHealth(), NULL, NODAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                            me->DespawnOrUnsummon(0);
                        }
                        else m_DeathTimer -= p_Diff;
                    }

                    int32 m_DeathTimer;
                };
            };
        }
    }
}

void AddSC_boss_forgemaster_gogduh()
{
    new MS::Instances::Bloodmaul::boss_forgemaster_gogduh();
    new MS::Instances::Bloodmaul::npc_ruination();
    new MS::Instances::Bloodmaul::npc_calamity();
    new MS::Instances::Bloodmaul::spell_rough_smash();
    new MS::Instances::Bloodmaul::spell_shatter_earth();
    new MS::Instances::Bloodmaul::areatrigger_shatter_earth();
    new MS::Instances::Bloodmaul::npc_shatter_earth();
}