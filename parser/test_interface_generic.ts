interface Container<T> {
    value: T;
    get(): T;
    set(value: T): void;
}
