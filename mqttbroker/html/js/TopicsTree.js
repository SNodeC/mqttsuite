function toggleGroup(id, btn) {
    var group = document.getElementById(id);
    if (group.style.display === 'none') {
        group.style.display = '';
        btn.textContent = '▼';
    } else {
        group.style.display = 'none';
        btn.textContent = '►';
    }
}

function trigger(topic) {
    alert('Unsubscribed from topic: ' + topic);
}
