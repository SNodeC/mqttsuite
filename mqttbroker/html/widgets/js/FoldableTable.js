// Expand/collapse logic
document.querySelectorAll('.key-row').forEach(row => {
  row.addEventListener('click', () => {
    row.classList.toggle('open');
    let next = row.nextElementSibling;
    while (next && next.classList.contains('value-row')) {
      next.style.display = row.classList.contains('open') ? 'table-row' : 'none';
      next = next.nextElementSibling;
    }
  });
});
