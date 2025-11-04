// Test interface parsing - basic features

// Basic interface with optional and readonly properties
interface Person {
    readonly id: number;
    name: string;
    age?: number;
    email?: string;
}

// Interface with index signature
interface StringDictionary {
    [key: string]: string;
    name: string;
}

// Interface with call signature
interface Callable {
    (param: string): boolean;
    description: string;
}

// Interface with construct signature
interface Constructor {
    new (value: number): any;
    staticMethod(): void;
}

// Interface inheritance
interface Employee extends Person {
    department: string;
    salary: number;
}

// Multiple inheritance
interface Manager extends Employee, Callable {
    teamSize: number;
}

// Interface with method signatures
interface Calculator {
    add(a: number, b: number): number;
    subtract(a: number, b: number): number;
    multiply?(a: number, b: number): number;
}

// Interface merging - same name declarations should merge
interface MergedInterface {
    prop1: string;
}

interface MergedInterface {
    prop2: number;
}
