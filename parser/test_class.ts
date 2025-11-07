// Test class parsing with generic after-name logic
class Person {
    name: string;
    age: number;
}

class Employee extends Person {
    department: string;
    salary: number;
}

class Manager extends Employee implements Printable {
    teamSize: number;

    print(): void {
        console.log("Manager");
    }
}

interface Printable {
    print(): void;
}
