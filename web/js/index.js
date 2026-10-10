const span_num = document.getElementsByClassName('num')[0];
let num = 0;
span_num.textContent = num; 
document.getElementsByClassName('add')[0].addEventListener('click', ()=>{
    console.log("点击");
    num+=1;
    span_num.textContent = num;
})