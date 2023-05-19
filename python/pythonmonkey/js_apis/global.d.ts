/// <reference no-default-lib="true"/>
/// <reference lib="es2022" />

declare function internalBinding(namespace: string): any; // catch-all

declare function internalBinding(namespace: "timers"): {
    /**
     * internal binding helper for the `setTimeout` global function
     * 
     * **UNSAFE**, does not perform argument type checks
     * @return timeoutId
     */
    enqueueWithDelay(handler: Function, delaySeconds: number): number;

    /**
     * internal binding helper for the `clearTimeout` global function
     */
    cancelByTimeoutId(timeoutId: number): void;
};

declare function internalBinding(namespace: "utils"): {
    defineGlobal(name: string, value: any): void;
};

declare const pythonBindings: ReadonlyArray<any>;
