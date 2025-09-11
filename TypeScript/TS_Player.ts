// YourProject/TypeScript/TS_Player

import * as UE from 'ue'

class TS_Player extends UE.Character {
    Constructor() {
        //...
        console.log("TS_Player Constructor called");
    }

    ReceiveBeginPlay(): void {
        console.log("TS_Player BeginPlay called");
    }
    ReceiveTick(InDeltaSeconds: number): void {
        //...
    }
    //...
}

export default TS_Player;