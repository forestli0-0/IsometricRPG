declare module "ue" {
  namespace UE {
    type TSubclassOf<T> = any;

    namespace uproperty {
      const EditAnywhere: any;
      const BlueprintReadWrite: any;
      function uproperty(...args: any[]): any;
    }

    class UObject {
      [key: string]: any;
    }

    class Class extends UObject {
      static Load(path: string): Class | null;
      StaticClass(): Class;
    }

    class Actor extends UObject {
      K2_DestroyActor(): void;
      K2_GetActorLocation(): Vector;
      GetComponentByClass(cls: Class): UObject | null;
      GetName(): string;
      Tags: any;
      static StaticClass(): Class;
    }

    class Pawn extends Actor {}

    class Character extends Pawn {
      static StaticClass(): Class;
    }

    class AIController extends Actor {
      RunBehaviorTree(tree: BehaviorTree): boolean;
      Pawn: Pawn;
      Blackboard: BlackboardComponent;
      GetWorld(): World;
      GetComponentByClass(cls: Class): UObject | null;
      GetName(): string;
      static StaticClass(): Class;
    }

    class IsometricEnemyCharacter extends Character {
      AbilitySystemComponent: AbilitySystemComponent;
      DefaultAbilities: any;
    }

    class IsometricAIController extends AIController {}

    class BTTask_TSBase extends UObject {
      FinishExecute(success: boolean): void;
    }

    class AIPerceptionComponent extends Actor {
      OnTargetPerceptionUpdated: { Add(cb: (actor: Actor, stimulus: AIStimulus) => void): void };
      GetOwner(): Actor;
      static StaticClass(): Class;
    }

    class AIStimulus extends UObject {
      bSuccessfullySensed: boolean;
      StimulusLocation: Vector;
    }

    class AbilitySystemComponent extends UObject {
      TryActivateAbilityByClass(abilityClass: Class, allowRemote?: boolean): boolean;
      static StaticClass(): Class;
    }

    class GameplayAbility extends UObject {}

    class Vector extends UObject {
      constructor(X?: number, Y?: number, Z?: number);
      static Dist(a: Vector, b: Vector): number;
      X: number;
      Y: number;
      Z: number;
    }

    class Rotator extends UObject {
      constructor(Pitch?: number, Yaw?: number, Roll?: number);
    }

    class Transform extends UObject {
      constructor(rot?: Rotator, loc?: Vector, scale?: Vector);
    }

    class BehaviorTree extends UObject {}

    class BlackboardComponent extends UObject {
      SetValueAsObject(key: string, value: any): void;
      SetValueAsVector(key: string, value: Vector): void;
      SetValueAsBool(key: string, value: boolean): void;
      GetValueAsObject(key: string): any;
    }

    class World extends UObject {}

    enum EEndPlayReason {}
    enum ESpawnActorCollisionHandlingMethod { AlwaysSpawn }
    enum EAutoPossessAI { PlacedInWorldOrSpawned }

    namespace GameplayStatics {
      function BeginDeferredActorSpawnFromClass(
        context: UObject,
        cls: Class,
        transform: Transform,
        collisionHandling?: ESpawnActorCollisionHandlingMethod,
        owner?: Actor | null,
        instigator?: any
      ): Actor | null;
      function FinishSpawningActor(actor: Actor, transform: Transform): void;
    }
  }
  export = UE;
}
