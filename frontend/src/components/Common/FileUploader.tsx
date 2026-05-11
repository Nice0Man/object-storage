import React, { useCallback } from "react";
import {
  Box,
  Paper,
  Typography,
  LinearProgress,
  List,
  ListItem,
  ListItemText,
  IconButton,
} from "@mui/material";
import { CloudUpload, Delete } from "@mui/icons-material";

interface FileUploaderProps {
  onFilesSelected: (files: File[]) => void;
  uploadProgress?: { [key: string]: number };
  multiple?: boolean;
  accept?: string;
}

const FileUploader: React.FC<FileUploaderProps> = ({
  onFilesSelected,
  uploadProgress = {},
  multiple = true,
  accept,
}) => {
  const [dragActive, setDragActive] = React.useState(false);
  const [selectedFiles, setSelectedFiles] = React.useState<File[]>([]);
  const fileInputRef = React.useRef<HTMLInputElement>(null);

  const handleDrag = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    if (e.type === "dragenter" || e.type === "dragover") {
      setDragActive(true);
    } else if (e.type === "dragleave") {
      setDragActive(false);
    }
  }, []);

  const handleDrop = useCallback(
    (e: React.DragEvent) => {
      e.preventDefault();
      e.stopPropagation();
      setDragActive(false);

      if (e.dataTransfer.files && e.dataTransfer.files.length > 0) {
        const files = Array.from(e.dataTransfer.files);
        setSelectedFiles(files);
        onFilesSelected(files);
      }
    },
    [onFilesSelected],
  );

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    if (e.target.files && e.target.files.length > 0) {
      const files = Array.from(e.target.files);
      setSelectedFiles(files);
      onFilesSelected(files);
    }
  };

  const handleClick = () => {
    fileInputRef.current?.click();
  };

  const handleRemoveFile = (index: number) => {
    const newFiles = selectedFiles.filter((_, i) => i !== index);
    setSelectedFiles(newFiles);
  };

  return (
    <Box>
      <Paper
        sx={{
          p: 4,
          textAlign: "center",
          border: "2px dashed",
          borderColor: dragActive ? "primary.main" : "grey.400",
          bgcolor: dragActive ? "action.hover" : "background.paper",
          cursor: "pointer",
          transition: "all 0.3s",
          "&:hover": {
            borderColor: "primary.main",
            bgcolor: "action.hover",
          },
        }}
        onDragEnter={handleDrag}
        onDragLeave={handleDrag}
        onDragOver={handleDrag}
        onDrop={handleDrop}
        onClick={handleClick}
      >
        <input
          ref={fileInputRef}
          type="file"
          multiple={multiple}
          accept={accept}
          onChange={handleChange}
          style={{ display: "none" }}
        />
        <CloudUpload sx={{ fontSize: 48, color: "primary.main", mb: 2 }} />
        <Typography variant="h6" gutterBottom>
          Drag & Drop files here
        </Typography>
        <Typography variant="body2" color="text.secondary">
          or click to browse
        </Typography>
      </Paper>

      {selectedFiles.length > 0 && (
        <Box sx={{ mt: 2 }}>
          <Typography variant="subtitle2" gutterBottom>
            Selected Files:
          </Typography>
          <List>
            {selectedFiles.map((file, index) => {
              const progress = uploadProgress[file.name] || 0;
              return (
                <ListItem
                  key={index}
                  secondaryAction={
                    progress === 0 && (
                      <IconButton
                        edge="end"
                        onClick={() => handleRemoveFile(index)}
                      >
                        <Delete />
                      </IconButton>
                    )
                  }
                  sx={{ flexDirection: "column", alignItems: "flex-start" }}
                >
                  <ListItemText
                    primary={file.name}
                    secondary={`${(file.size / 1024 / 1024).toFixed(2)} MB`}
                  />
                  {progress > 0 && (
                    <Box sx={{ width: "100%", mt: 1 }}>
                      <LinearProgress
                        variant="determinate"
                        value={progress}
                      />
                      <Typography variant="caption" sx={{ mt: 0.5 }}>
                        {progress}%
                      </Typography>
                    </Box>
                  )}
                </ListItem>
              );
            })}
          </List>
        </Box>
      )}
    </Box>
  );
};

export default FileUploader;
