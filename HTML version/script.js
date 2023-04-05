// Define a function to handle the start game button click event
function startGame() {
  window.location.href = "character.html";
}

// Define a function to handle the character selection form submission
function selectCharacter(event) {
  event.preventDefault();
  const character = document.querySelector('input[name="character"]:checked').value;
  // Use the selected character value to customize the game experience
  window.location.href = "game.html";
}

// Attach event listeners to the appropriate elements
document.getElementById("start-btn").addEventListener("click", startGame);
document.querySelector("form").addEventListener("submit", selectCharacter);
