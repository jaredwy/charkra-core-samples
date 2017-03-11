var a = 10;
console.log("Rar");
console.log("Rar", "With", "a", "few", "params");
console.log("a is set to " +a);
console.log(setTimeout(() => console.log("Rar0"), 0));
console.log(setTimeout(() => console.log("Rar1"), 1000));
console.log(setTimeout(() => { console.log("Rar3"); application.quit() }, 3000));
console.log(setTimeout(() => console.log("Rar2"), 2000));
console.log(setTimeout(() => console.log("Rar00"), 0));

