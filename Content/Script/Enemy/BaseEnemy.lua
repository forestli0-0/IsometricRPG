---@diagnostic disable: undefined-global
-- 基础敌人 Lua 行为，叠加在 C++ AIsometricEnemyCharacter 之上
local BaseEnemy = Class()

function BaseEnemy:ReceiveBeginPlay()
    if not self.AbilitySystemComponent then
        print(string.format("[BaseEnemy] Warning: ASC missing on %s (check C++ ctor)", self:GetName()))
    end
    self.Overridden.ReceiveBeginPlay(self)
end

-- 返回主要的攻击技能类，从 C++ 的 DefaultAbilities 读取
function BaseEnemy:GetAttackAbilityClass()
    local abilities = self.DefaultAbilities
    if abilities then
        if abilities.Num and abilities:Num() > 0 then
            -- 先尝试 1 基坐标（UnLua 常见），失败再用 0 基的 Get(0)
            local cls = abilities:Get(1) or abilities:Get(0)
            if cls then
                return cls
            end
        elseif abilities[1] then
            return abilities[1]
        end
    end
    return nil
end

return BaseEnemy
