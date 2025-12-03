import * as UE from "ue";

// 这是一个抽象的敌人基类，所有具体的敌人（近战、远程）都应该继承它（或它的蓝图子类）
// 继承自 C++ 的 IsometricEnemyCharacter，因为它已经在 C++ 构造函数中创建了 ASC 并完成了 InitAbilityActorInfo
class TS_BaseEnemy extends UE.IsometricEnemyCharacter {
    
    // --- 配置项 (在蓝图 Defaults 里修改) ---

    ReceiveBeginPlay(): void {
        // C++ 父类的 BeginPlay 会自动运行并初始化 GAS (InitAbilityActorInfo)
        if (!this.AbilitySystemComponent) {
            console.warn(`[TS_BaseEnemy] Warning: AbilitySystemComponent is missing on ${this.GetName()}! Make sure C++ constructor created it.`);
        }
    }

    // --- 接口 ---

    // 获取普攻技能类
    public GetAttackAbilityClass(): UE.TSubclassOf<UE.GameplayAbility> {
        // 尝试从 C++ 的 DefaultAbilities 获取第一个技能作为普攻
        if (this.DefaultAbilities && this.DefaultAbilities.Num() > 0) {
            return this.DefaultAbilities.Get(0) as unknown as UE.TSubclassOf<UE.GameplayAbility>;
        }
        return null;
    }
}

export default TS_BaseEnemy;
