---@diagnostic disable: undefined-global
-- 寻找远离目标位置的任务节点
local BTT_FindFleeLocation = Class()

-- 可调参数
BTT_FindFleeLocation.FleeDistance = 600
BTT_FindFleeLocation.BlackboardKey_Target = "TargetActor"
BTT_FindFleeLocation.BlackboardKey_Result = "FleeLocation"

function BTT_FindFleeLocation:ReceiveExecuteAI(OwnerController, ControlledPawn)
    local bb = OwnerController.Blackboard
    if not bb then
        self:FinishExecute(false)
        return
    end

    local target = bb:GetValueAsObject(self.BlackboardKey_Target)
    if not target then
        -- 没有目标，无需逃跑
        self:FinishExecute(false)
        return
    end

    local pawnLoc = ControlledPawn:K2_GetActorLocation()
    local targetLoc = target:K2_GetActorLocation()

    -- 计算从目标指向自己的向量（逃跑方向）
    local fleeDir = pawnLoc - targetLoc
    fleeDir.Z = 0 -- 忽略高度差
    
    -- 归一化
    local len = math.sqrt(fleeDir.X * fleeDir.X + fleeDir.Y * fleeDir.Y)
    if len > 0 then
        fleeDir = fleeDir / len
    else
        -- 如果位置重叠，随机选择一个方向
        fleeDir = UE.FVector(math.random() * 2 - 1, math.random() * 2 - 1, 0)
        local rLen = math.sqrt(fleeDir.X * fleeDir.X + fleeDir.Y * fleeDir.Y)
        if rLen > 0 then fleeDir = fleeDir / rLen end
    end

    -- 计算期望位置
    local dist = self.FleeDistance or 600
    local desiredLoc = pawnLoc + fleeDir * dist

    -- 投射到导航系统以确保位置可达
    local outResult = UE.FVector()
    local bSuccess = UE.UNavigationSystemV1.K2_GetRandomReachablePointInRadius(ControlledPawn, desiredLoc, outResult, 200)
    
    if bSuccess then
        bb:SetValueAsVector(self.BlackboardKey_Result, outResult)
        self:FinishExecute(true)
        return
    end

    -- 备选方案：直接设置计算出的原始点（如果不在导航网格上，MoveTo 可能会失败，但总比没有好）
    bb:SetValueAsVector(self.BlackboardKey_Result, desiredLoc)
    self:FinishExecute(true)
end

return BTT_FindFleeLocation
