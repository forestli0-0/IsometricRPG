declare module "puerts" {
  export namespace blueprint {
    function tojs<T = any>(cls: any): T;
    function mixin(target: any, source: any): void;
  }
}
