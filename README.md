# Smart Bird Feeder
## Group 5
### IoT Project TX00EX87-3001


IoT Project for Metropolia University of Applied Sciences, 2024.

This project aims to develop a Smart Bird Feeder system that incorporates IoT technology to monitor and analyze bird activity remotely. The system will provide features such as data collection on bird visits, and integration with cloud-based analytics. It will also include hardware design (PCB and enclosure) and software components (firmware and server).

## Features

- **Pest Prevention**: Locks the feeding bay when closed to prevent pests from accessing food. Using a presence sensor, the feeder remains locked until the pest is gone
- **Remote Connectivity**: Utilizes LoRaWAN for transmitting data to a central server.
- **Real-Time Updates**: Provides live data on locking events and the ability to lock/unlock through a user interface.
- **Energy Efficiency**: Optimized for low power consumption to support remote deployment.
- **Scalable Design**: Supports future integration with additional sensors or devices.

## System Components

1. **Hardware**:
   - Sensors: Micro Switch, ToF Presence Sensor
   - Actuator: Servo motor
   - LoRa module for wireless communication.
   - Custom-designed PCB.
   - 3D-printed bird feeder enclosure.

2. **Firmware**:
   - Developed using FreeRTOS for task scheduling.
   - Includes modules for sensor data collection, LoRa communication, and power management.

3. **Server**:
   - Processes and stores data received from the bird feeder.
   - Hosts a web-based interface for monitoring and visualization.

## Getting Started

Follow these steps to set up and contribute to the project:

### Cloning the Repository
1. Clone the repository to your local machine:
   ```bash
	git clone https://gitlab.metropolia.fi/abhinavk/smart-bird-feeder.git
	```
2. Navigate to the project directory:
   ```bash
	cd <local repo>
	```
3. Set the upstream remote (if not already set):
   ```bash
	git remote add upstream https://gitlab.metropolia.fi/abhinavk/smart-bird-feeder.git
	```

### Initializing and Syncing Your Local Repository
1. Pull the latest changes from the main branch:
   ```bash
	git pull upstream main
	```
2. Create a new feature branch for your work:
   ```bash
	git checkout -b feature/your-feature-name
	```

## Feature Branch Workflow

To ensure a clean and collaborative development process, the team will use the **Feature Branch Workflow**. Follow these best practices:

1. **Create a Feature Branch**:
   - Name your branch descriptively based on the feature or task (e.g., `feature/add-authentication` or `bugfix/fix-ui-issue`).

2. **Commit Regularly**:
   - Make small, incremental commits with meaningful messages.

3. **Push Your Branch**:
   - Push your branch to the remote repository:
   ```bash
	git push origin feature/your-feature-name
	```

4. **Create a Merge Request**:
   - When your feature is complete and tested, create a merge request from your feature branch to the `main` branch.

5. **Code Reviews**:
   - Ensure your merge request is reviewed and approved by at least one team member before merging.

6. **Keep Your Branch Updated**:
   - Regularly pull updates from `main` to your feature branch to avoid conflicts:
   ```bash
	git pull upstream main
	```

## Contributing Guidelines

- Follow coding standards and document your code properly.
- Include detailed commit messages and documentation for all new features.
- Write and test code thoroughly before submitting a merge request.

## Contact and Support

For any questions or support, please reach out to the project team through the GitLab repository issue tracker.
