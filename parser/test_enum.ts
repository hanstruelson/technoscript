// Test enum parsing
enum Color {
    Red,
    Green,
    Blue
}

enum Direction {
    Up = 1,
    Down,
    Left,
    Right
}

enum Status {
    Active = "ACTIVE",
    Inactive = "INACTIVE",
    Pending = "PENDING"
}

enum Mixed {
    First = 1,
    Second = "SECOND",
    Third = 3
}

const enum ConstEnum {
    A,
    B,
    C
}
