const overlay = document.getElementById('spinner-overlay')

function showSpinner() {
    if (!overlay.classList.contains('active')) {
        overlay.classList.add('active')
    }
}

function hideSpinner() {
    if (overlay.classList.contains('active')) {
        overlay.classList.remove('active')
    }
}

window.addEventListener('load', function() {
    hideSpinner()
});

showSpinner()
