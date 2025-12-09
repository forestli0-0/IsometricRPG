-- 1. 创建类 (对应 BTTask_BlueprintBase)
local BTT_GenericAttack = Class()

-- 2. 重写 ReceiveExecuteAI
-- 参数：OwnerController (AIController), ControlledPawn (Pawn)
function BTT_GenericAttack:ReceiveExecuteAI(OwnerController, ControlledPawn)
    
    -- ---------------------------------------------------------
    -- 步骤 1: 获取技能类
    -- ---------------------------------------------------------
    local AbilityClass = nil

    -- Lua 中调用 C++ 函数用冒号 :
    if ControlledPawn.GetAttackAbilityClass then
        AbilityClass = ControlledPawn:GetAttackAbilityClass()
    end

    -- 兼容：如果上面没获取到，尝试直接读取属性 (假设是反射暴露的属性)
    if not AbilityClass and ControlledPawn.AttackAbilityClass then
        AbilityClass = ControlledPawn.AttackAbilityClass
    end

    if not AbilityClass then
        print(string.format("[BTT_GenericAttack] Pawn %s 没配置攻击技能! 请检查 DefaultAbilities。", ControlledPawn:GetName()))
        self:FinishExecute(false)
        return
    end

    -- ---------------------------------------------------------
    -- 步骤 2: 准备目标数据
    -- ---------------------------------------------------------
    local Blackboard = OwnerController.Blackboard
    local TargetActor = nil
    
    if Blackboard then
        -- UnLua 获取黑板数据，Key 可以直接传字符串
        TargetActor = Blackboard:GetValueAsObject("TargetActor")
    end

    if TargetActor then
        -- 调用 C++ 设置目标数据的函数
        if ControlledPawn.SetAbilityTargetDataByActor then
            ControlledPawn:SetAbilityTargetDataByActor(TargetActor)
            -- print("Lua: 目标数据已设置为 " .. TargetActor:GetName())
        else
            print(string.format("[BTT_GenericAttack] Pawn %s 不支持 SetAbilityTargetDataByActor!", ControlledPawn:GetName()))
        end
    else
        print("[BTT_GenericAttack] 黑板里没有 TargetActor!")
    end

    -- ---------------------------------------------------------
    -- 步骤 3: 激活 GAS 技能
    -- ---------------------------------------------------------
    -- 尝试获取 ASC (AbilitySystemComponent)
    -- 优先使用接口获取
    local ASC = ControlledPawn:GetAbilitySystemComponent()

    -- 如果接口拿不到，尝试通过组件查找
    if not ASC then
        ASC = ControlledPawn:GetComponentByClass(UE.UAbilitySystemComponent.StaticClass())
    end

    if ASC then
        -- print("[BTT_GenericAttack] 获取到 ASC: " .. tostring(ASC))
        
        -- 激活技能
        -- 这里的 true 表示 bAllowRemoteActivation
        local bActivated = ASC:TryActivateAbilityByClass(AbilityClass, true)

        if bActivated then
            -- 激活成功
            self:FinishExecute(true)
        else
            -- 激活失败 (可能冷却中，或者 Tag 限制)
            -- 返回 true 让行为树认为任务完成了（防止卡死），进入后续的 Wait 节点
            -- print("[BTT_GenericAttack] 技能未激活 (可能在冷却)，跳过。")
            self:FinishExecute(true)
        end
    else
        print("[BTT_GenericAttack] 找不到 AbilitySystemComponent!")
        self:FinishExecute(false)
    end
end

return BTT_GenericAttack