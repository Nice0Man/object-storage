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
import { widgetScrollSx } from "../../theme/widgetStyles";

const formatFileSize = (bytes: number): string => {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(2)} KB`;
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / 1024 / 1024).toFixed(2)} MB`;
  return `${(bytes / 1024 / 1024 / 1024).toFixed(2)} GB`;
};

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
          <List
            dense
            sx={(theme) => ({
              maxHeight: 280,
              overflowY: "auto",
              overflowX: "hidden",
              border: 1,
              borderColor: "divider",
              borderRadius: 1,
              ...widgetScrollSx(theme),
            })}
          >
            {selectedFiles.map((file, index) => {
              const progress = uploadProgress[file.name] || 0;
              return (
                <ListItem
                  key={`${file.name}-${index}`}
                  alignItems="flex-start"
                  sx={{ pr: progress === 0 ? 7 : 2, py: 1 }}
                  secondaryAction={
                    progress === 0 ? (
                      <IconButton edge="end" aria-label="Remove file" onClick={() => handleRemoveFile(index)}>
                        <Delete />
                      </IconButton>
                    ) : undefined
                  }
                >
                  <ListItemText
                    primary={
                      <Typography variant="body2" noWrap title={file.name}>
                        {file.name}
                      </Typography>
                    }
                    secondary={
                      <Box component="span" sx={{ display: "block", width: "100%", mt: 0.25 }}>
                        <Typography variant="caption" color="text.secondary" component="span">
                          {formatFileSize(file.size)}
                        </Typography>
                        {progress > 0 && (
                          <Box sx={{ width: "100%", mt: 1, pr: 1 }}>
                            <LinearProgress variant="determinate" value={progress} sx={{ height: 6, borderRadius: 1 }} />
                            <Typography variant="caption" color="text.secondary" sx={{ mt: 0.5, display: "block" }}>
                              {progress}%
                            </Typography>
                          </Box>
                        )}
                      </Box>
                    }
                  />
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
