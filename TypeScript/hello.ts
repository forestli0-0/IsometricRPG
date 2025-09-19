import * as UE from 'ue'

class hello extends UE.Actor {
    Constructor() {
        console.log("hello Constructor called");
    }
    ReceiveBeginPlay(): void {
        console.log("hello BeginPlay called");
    }
    
    ReceiveTick(InDeltaSeconds: number): void {
        //...
        // console.log("hello Tick called");

    }
    //...
}
export default hello;