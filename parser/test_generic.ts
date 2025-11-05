function test<T>(value: string) {
    return value;
}

function multiple<T, U>(a: T, b: U): T {
    return a;
}

// Generic interface
interface Container<T> {
    value: T;
    get(): T;
    set(value: T): void;
}

// Generic interface with multiple parameters
interface Dictionary<K, V> {
    get(key: K): V;
    set(key: K, value: V): void;
    has(key: K): boolean;
}

// Simple class for testing
class Simple {
    test(): void {
        console.log("test");
    }
}

// Generic class
class Stack<T> {
    private items: T[] = [];

    push(item: T): void {
        this.items.push(item);
    }

    pop(): T | undefined {
        return this.items.pop();
    }

    peek(): T | undefined {
        return this.items[this.items.length - 1];
    }

    isEmpty(): boolean {
        return this.items.length === 0;
    }
}

// Generic class with multiple parameters
class Pair<K, V> {
    key: K;
    value: V;

    toString(): string {
        return "test";
    }
}

// Generic class extending another generic class
class ExtendedStack<T> {
    size(): number {
        return 0;
    }
}
