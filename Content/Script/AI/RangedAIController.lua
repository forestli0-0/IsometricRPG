---@diagnostic disable: undefined-global

local BaseController = require("AI.EnemyAIController")

if not BaseController then
    print("Lua Error: BaseController is nil! Check AI/EnemyAIController.lua return value.")
end

local RangedAIController = Class("AI.EnemyAIController")

-- 为远程行为新增的黑板键
local BBKEY_TOO_CLOSE = "bTooClose"

-- 配置：保持距离的最小值，低于此值就尝试后退
RangedAIController.MinKeepDistance = 600 -- 如果目标距离小于这个值，就尝试后退

function RangedAIController:ReceiveBeginPlay()
    if BaseController and BaseController.ReceiveBeginPlay then
        BaseController.ReceiveBeginPlay(self)
    end
end

function RangedAIController:ReceiveTick(DeltaSeconds)
    -- 先调用父类逻辑（处理目标更新、攻击范围检查等）
    if BaseController and BaseController.ReceiveTick then
        BaseController.ReceiveTick(self, DeltaSeconds)
    end

    local bb = self.Blackboard
    if not bb then return end

    local target = bb:GetValueAsObject("TargetActor")
    if target then
        local pawn = self.Pawn
        if pawn then
            local dist = pawn:GetDistanceTo(target)
            
            -- 检查是否距离过近
            local bTooClose = dist < self.MinKeepDistance
            bb:SetValueAsBool(BBKEY_TOO_CLOSE, bTooClose)
        end
    else
        -- 没有目标时重置状态
        bb:SetValueAsBool(BBKEY_TOO_CLOSE, false)
    end
end

return RangedAIController
