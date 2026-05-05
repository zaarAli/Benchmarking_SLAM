import pandas as pd
import matplotlib.pyplot as plt

# Load the raw data logs into pandas DataFrame
def load_log(file_path):
    # Define columns for the data
    columns = ['Time', 'UID', 'PID', 'usr', 'system', 'CPU', 'CPU_percent', 'minflt/s', 'majflt/s', 'VSZ', 'RSS', 'Command']
    
    # Load the data, skipping the first few non-data lines
    df = pd.read_csv(file_path, delim_whitespace=True, header=None, names=columns)
    
    # Convert time to datetime format
    df['Time'] = pd.to_datetime(df['Time'], errors='coerce')
    
    # Filter the data for just the SLAM process (e.g., "hector_mapping")
    df = df[df['Command'].str.contains('hector_mapping', case=False, na=False)]
    
    # Drop any rows where Time couldn't be converted
    df.dropna(subset=['Time'], inplace=True)
    
    return df

# Plotting function
def plot_usage(df, output_filename):
    plt.figure(figsize=(10, 6))
    
    # Plot CPU usage and RSS (memory)
    plt.plot(df['Time'], df['CPU_percent'], label='CPU Usage (%)', color='tab:red')
    plt.plot(df['Time'], df['RSS'], label='Memory Usage (RSS) in KB', color='tab:blue')
    
    # Format the plot
    plt.title("SLAM Usage (CPU & Memory)")
    plt.xlabel("Time")
    plt.ylabel("Usage")
    plt.legend()
    plt.xticks(rotation=45)
    plt.tight_layout()
    
    # Save the plot to a file
    plt.savefig(output_filename)
    plt.show()

# Main function
def main():
    raw_log_file = 'hector_pidstat_raw.log'  # Path to the raw log file
    usage_log_file = 'hector_usage.log'     # Path to the processed log file
    
    # Load the logs
    raw_data = load_log(raw_log_file)
    
    # Plot the data
    plot_usage(raw_data, 'hector_usage_graph.png')

if __name__ == "__main__":
    main()
