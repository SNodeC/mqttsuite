const overlay = document.getElementById('spinner-overlay')

function showSpinner() {
    console.log("Show Spinner")
    overlay.classList.add('active')
}

function hideSpinner() {
    console.log("Hide Spinner")
    // No need to hide spinner as the page is reloaded anyway
     overlay.classList.remove('active')
}

window.addEventListener('load', function() {
    if (typeof hideSpinner == 'function') {
        hideSpinner?.()
    }
});
