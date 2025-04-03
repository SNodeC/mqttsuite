const splitter = document.getElementById('splitter');
const topDiv = document.getElementById('drag');
const container = document.getElementById('container');
let isDragging = false;
splitter.addEventListener('mousedown', function(e) {
    e.preventDefault();
    isDragging = true;
    document.body.style.cursor = 'row-resize';
});
document.addEventListener('mousemove', function(e) {
    e.preventDefault();
    if (!isDragging) return;
    const containerRect = container.getBoundingClientRect();
    const newHeight = 100 *  (e.clientY  - containerRect.top - 22) / containerRect.height;
    topDiv.style.height = `${newHeight}%`;
});
document.addEventListener('mouseup', function() {
    isDragging = false;
    document.body.style.cursor = 'default';
});
