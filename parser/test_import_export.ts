// Test import/export statements
import { foo, bar as baz } from 'module1';
import * as utils from 'module2';
import defaultExport from 'module3';
import defaultExport2, { named1, named2 as alias2 } from 'module4';

export { foo, bar as baz };
export * from 'module5';
export * as all from 'module6';
export default function() {};
export const x = 1;
export let y = 2;
export var z = 3;
export function test() {}
