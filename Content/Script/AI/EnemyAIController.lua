---@diagnostic disable: undefined-global
-- 敌人用的 Lua AI 控制器，将数据更新从蓝图中分离，组件由 C++ 持有
local EnemyAIController = Class()

-- 黑板键（必须与行为树配置保持一致）
local BBKEY_SELF = "SelfActor"
local BBKEY_TARGET = "TargetActor"
local BBKEY_LAST_LOC = "LastKnownLocation"
local BBKEY_IN_RANGE = "bIsInAttackRange"

-- 可调参数
EnemyAIController.UpdateInterval = 0.25
EnemyAIController.EnableDebug = false
EnemyAIController.LoseSightDistance = 2000 -- 为保持一致性而保留，当前未使用

function EnemyAIController:ReceiveBeginPlay()
    -- 若已指定行为树则启动
    if self.BehaviorTreeAsset then
        local ok = self:RunBehaviorTree(self.BehaviorTreeAsset)
        if self.EnableDebug then
            print(string.format("[AI] BT Start: %s", tostring(ok)))
        end
    end

    local pawn = self.Pawn
    local bb = self.Blackboard
    if pawn and bb then
        bb:SetValueAsObject(BBKEY_SELF, pawn)
    end

    -- 绑定感知更新（优先使用控制器组件，失败则回退到 Pawn）
    local perception = self:GetComponentByClass(UE.UAIPerceptionComponent.StaticClass())
    if not perception and pawn then
        perception = pawn:GetComponentByClass(UE.UAIPerceptionComponent.StaticClass())
    end

    if perception and perception.OnTargetPerceptionUpdated then
        perception.OnTargetPerceptionUpdated:Add(self, self.OnPerceptionUpdated)
        if self.EnableDebug then
            local ownerName = perception:GetOwner() and perception:GetOwner():GetName() or "?"
            print(string.format("[AI] Perception bound (%s)", ownerName))
        end
    else
        print("[AI] Warning: No AIPerceptionComponent found on Controller or Pawn!")
    end

end

function EnemyAIController:ReceiveTick(DeltaSeconds)
    self._accum = (self._accum or 0) + DeltaSeconds
    if self._accum < (self.UpdateInterval or 0.25) then
        return
    end
    self._accum = 0

    local bb = self.Blackboard
    local pawn = self.Pawn
    if not bb or not pawn then
        return
    end

    local target = bb:GetValueAsObject(BBKEY_TARGET)
    if target then
        local tLoc = target:K2_GetActorLocation()
        bb:SetValueAsVector(BBKEY_LAST_LOC, tLoc)

        local pLoc = pawn:K2_GetActorLocation()
        local dx = pLoc.X - tLoc.X
        local dy = pLoc.Y - tLoc.Y
        local dz = pLoc.Z - tLoc.Z
        local dist = math.sqrt(dx * dx + dy * dy + dz * dz)

        local attackRange = 200
        if pawn.GetAttackRange then
            attackRange = pawn:GetAttackRange()
        end

        local inRange = dist <= (attackRange + 50) -- 为胶囊体留一点误差
        bb:SetValueAsBool(BBKEY_IN_RANGE, inRange)
    else
        bb:SetValueAsBool(BBKEY_IN_RANGE, false)
    end
end

function EnemyAIController:HandlePerceptionUpdated(Actor, Stimulus)
    local bb = self.Blackboard
    local pawn = self.Pawn
    if not bb or not pawn then
        return
    end

    if Stimulus and Stimulus.bSuccessfullySensed then
        local currentTarget = bb:GetValueAsObject(BBKEY_TARGET)
        -- 只有当前没有目标，或者感知到的就是当前目标时，才更新（实现目标锁定）
        if not currentTarget or currentTarget == Actor then
            local targetLoc = Actor and Actor:K2_GetActorLocation() or (Stimulus.StimulusLocation or UE.FVector())
            bb:SetValueAsObject(BBKEY_TARGET, Actor)
            bb:SetValueAsVector(BBKEY_LAST_LOC, targetLoc)
            if self.EnableDebug and Actor then
                print(string.format("[AI] Sensed (Locked): %s", Actor:GetName()))
            end
        end
    else
        -- 失去视野
        if Stimulus and Stimulus.StimulusLocation then
            bb:SetValueAsVector(BBKEY_LAST_LOC, Stimulus.StimulusLocation)
        end
        local currentTarget = bb:GetValueAsObject(BBKEY_TARGET)
        if currentTarget == Actor then
            bb:SetValueAsObject(BBKEY_TARGET, nil)
            bb:SetValueAsBool(BBKEY_IN_RANGE, false)
            if self.EnableDebug and Actor then
                print(string.format("[AI] Lost: %s", Actor:GetName()))
            end
        end
    end
end

function EnemyAIController:OnPerceptionUpdated(Actor, Stimulus)
    self:HandlePerceptionUpdated(Actor, Stimulus)
end

return EnemyAIController
