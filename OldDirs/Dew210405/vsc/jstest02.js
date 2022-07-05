// JavaScript source code
// jstest02.js

'use strict';

//  console.log("-- begin main() --");

class Rectangle {
    constructor(height, width) {
      this.height = height;
      this.width = width;
    }
    // Getter
    get area() {
      return this.calcArea();
    }
    // Method
    calcArea() {
      return this.height * this.width;
    }
     // Method
     setit() {
        this.height = this.height * 2;
        this.width = this.width * 2;
      }
   }
  
  var square = new Rectangle(10, 10);

function func4(aa) {
    aa.setit();
    console.log("-- func4() aa=", aa);
}
func4(square);
  
  console.log(square.area); // 100

  console.log("----------------"); 

var var01;
function func0(aa) {
    aa=aa + 'five';
    console.log("-- func0() aa=", aa);
}
var01='one';
func0(var01);
console.log("-- main() var01=", var01);
