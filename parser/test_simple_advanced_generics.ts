// Test file for simple advanced TypeScript generics features

// Conditional types
type IsString<T> = T extends string ? true : false;
type Test1 = IsString<string>;  // true
type Test2 = IsString<number>;  // false

// Infer keyword in conditional types
type MyReturnType<T> = T extends (...args: any[]) => infer R ? R : any;
type Test3 = MyReturnType<() => string>;  // string

// Template literal types
type EventName<T extends string> = `on${Capitalize<T>}`;
type Test4 = EventName<'click'>;  // 'onClick'

// Mapped types
type Optional<T> = {
    [K in keyof T]?: T[K];
};
type Test5 = Optional<{a: number, b: string}>;  // {a?: number, b?: string}

// Recursive conditional types
type DeepReadonly<T> = {
    readonly [K in keyof T]: T[K] extends object ? DeepReadonly<T[K]> : T[K];
};
type Test6 = DeepReadonly<{a: {b: number}}>;  // {readonly a: {readonly b: number}}

// Complex conditional types with infer
type ElementType<T> = T extends (infer U)[] ? U : never;
type Test7 = ElementType<string[]>;  // string

// Nested conditional types
type Flatten<T> = T extends (infer U)[] ? Flatten<U> : T;
type Test8 = Flatten<number[][]>;  // number

// Template literal types with unions
type CSSProperties = 'color' | 'background' | 'border';
type CSSValues<T extends CSSProperties> = T extends 'color' ? string : T extends 'background' ? string : number;
type Test9 = CSSValues<'color'>;  // string

// Conditional types with template literals
type APIEndpoint<T extends string> = T extends `${infer Method} ${infer Path}` ? {method: Method, path: Path} : never;
type Test10 = APIEndpoint<'GET /users'>;  // {method: 'GET', path: '/users'}

// Recursive types with conditional types
type JSONValue = string | number | boolean | null | JSONArray | JSONObject;
type JSONArray = JSONValue[];
type JSONObject = {
    [K in string]: JSONValue;
};

// Complex conditional type with multiple inferences
type FunctionParameters<T> = T extends (...args: infer P) => any ? P : never;
type Test11 = FunctionParameters<(a: string, b: number) => void>;  // [string, number]

// Mapped types with readonly modifier
type ReadonlyRecord<T> = {
    readonly [K in keyof T]: T[K];
};
type Test12 = ReadonlyRecord<{a: number, b: string}>;  // {readonly a: number, readonly b: string}

// Template literal types with conditional constraints
type HTTPMethod = 'GET' | 'POST' | 'PUT' | 'DELETE';
type Route<T extends string> = T extends `/${string}` ? T : never;
type Test13 = Route<'/users'>;  // '/users'

// Conditional types with never type
type MyExclude<T, U> = T extends U ? never : T;
type Test14 = MyExclude<'a' | 'b' | 'c', 'a' | 'b'>;  // 'c'

// Advanced recursive conditional types
type DeepPartial<T> = T extends object ? { [K in keyof T]?: DeepPartial<T[K]> } : T;
type Test15 = DeepPartial<{a: {b: number}}>;  // simplified

// Template literal types with inference
type ParseURL<T extends string> = T extends `${infer Protocol}://${infer Domain}/${infer Path}` ? {protocol: Protocol, domain: Domain, path: Path} : never;
type Test16 = ParseURL<'https://example.com/users'>;  // {protocol: 'https', domain: 'example.com', path: 'users'}
